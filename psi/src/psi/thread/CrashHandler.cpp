#include "psi/thread/CrashHandler.h"

#ifdef __linux__
    #include <execinfo.h>
    #include <sys/resource.h>
#elif _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <Winternl.h>
    #include <Shellapi.h>
    #include <conio.h>
    #include <dbghelp.h>
    #include <exception>
    #include <new.h>
    #include <psapi.h>
    #include <rtcapi.h>
    #include <signal.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <sys/stat.h>
    #include <tchar.h>
#endif

#include <array>
#include <iostream>
#include <memory>
#include <signal.h>
#include <sstream>

#include "psi/tools/Tools.h"

#ifdef PSI_LOGGER
#include "psi/logger/Logger.h"
#else
#include <iostream>
#include <sstream>
#define LOG_INFO_STATIC(x)                                                                                             \
    do {                                                                                                               \
        std::ostringstream os;                                                                                         \
        os << x;                                                                                                       \
        std::cout << os.str() << std::endl;                                                                            \
    } while (0)
#define LOG_ERROR_STATIC(x) LOG_INFO_STATIC(x)
#endif

namespace psi::thread {

#ifdef __linux__
std::string executeCommand(const std::string &cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}
#endif

#ifdef _WIN32
class symbol
{
    IMAGEHLP_SYMBOL64 *sym;
    const int maxNameLen = 1024;

public:
    symbol(HANDLE process, DWORD64 address)
    {
        sym = (IMAGEHLP_SYMBOL64 *)malloc(sizeof(*sym) + maxNameLen);
        std::memset(sym, '\0', sizeof(*sym) + maxNameLen);
        sym->SizeOfStruct = sizeof(*sym);
        sym->MaxNameLength = maxNameLen;
        DWORD64 displacement;
        SymGetSymFromAddr64(process, address, &displacement, sym);
    }

    ~symbol()
    {
        free(sym);
    }

    std::string name()
    {
        return std::string(sym->Name);
    }

    std::string unDecoratedNme()
    {
        if (*sym->Name == '\0') {
            return "Unknown";
        }
        std::vector<char> undName(maxNameLen);
        UnDecorateSymbolName(sym->Name, &undName[0], maxNameLen, UNDNAME_COMPLETE);
        return std::string(&undName[0], strlen(&undName[0]));
    }
};

std::string convertExceptionCodeToString(DWORD exCode)
{
    auto hLibrary = LoadLibrary(_T("ntdll.dll"));
    if (hLibrary == NULL) {
        LOG_ERROR_STATIC("Couldn't load ntdll.dll");
        return "";
    }

    LPSTR temp;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
                  hLibrary,
                  RtlNtStatusToDosError(exCode),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&temp,
                  0,
                  NULL);
    FreeLibrary(hLibrary);

    auto result = std::string(temp);

    return tools::trim(result);
}

const std::string createStacktrace(EXCEPTION_POINTERS *exceptionPtr)
{
    const auto currentProcess = GetCurrentProcess();

    // Load psapi.dll
    auto hPsApi = LoadLibrary(_T("psapi.dll"));
    if (hPsApi == nullptr) {
        LOG_ERROR_STATIC("Couldn't load psapi.dll");
        return "";
    }

    if (!SymInitialize(currentProcess, NULL, false)) {
        return "No stacktrace";
    }

    DWORD symOptions = SymGetOptions();
    symOptions |= SYMOPT_LOAD_LINES | SYMOPT_UNDNAME;
    SymSetOptions(symOptions);

    std::vector<HMODULE> module_handles(1);
    DWORD cbNeeded;
    EnumProcessModules(currentProcess, &module_handles[0], DWORD(module_handles.size() * sizeof(HMODULE)), &cbNeeded);
    module_handles.resize(cbNeeded / sizeof(HMODULE));
    EnumProcessModules(currentProcess, &module_handles[0], DWORD(module_handles.size() * sizeof(HMODULE)), &cbNeeded);

    struct ModuleData {
        std::string imageName;
        std::string moduleName;
        void *baseAddress;
        DWORD loadSize;
    };
    std::vector<ModuleData> modules;

    auto getModuleinfo = [currentProcess](HMODULE module) -> ModuleData {
        static const int buffer_length = 4096;

        ModuleData ret;
        char temp[buffer_length];
        MODULEINFO mi;

        GetModuleInformation(currentProcess, module, &mi, sizeof(mi));
        ret.baseAddress = mi.lpBaseOfDll;
        ret.loadSize = mi.SizeOfImage;

        GetModuleFileNameEx(currentProcess, module, temp, sizeof(temp));
        ret.imageName = temp;
        GetModuleBaseName(currentProcess, module, temp, sizeof(temp));
        ret.moduleName = temp;
        std::vector<char> img(ret.imageName.begin(), ret.imageName.end());
        std::vector<char> mod(ret.moduleName.begin(), ret.moduleName.end());
        SymLoadModule64(currentProcess, 0, &img[0], &mod[0], (DWORD64)ret.baseAddress, ret.loadSize);
        return ret;
    };
    std::transform(module_handles.begin(), module_handles.end(), std::back_inserter(modules), getModuleinfo);

    CONTEXT *context = exceptionPtr->ContextRecord;
#ifdef _M_X64
    STACKFRAME64 frame;
    frame.AddrPC.Offset = context->Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context->Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context->Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
#else
    STACKFRAME64 frame;
    frame.AddrPC.Offset = context->Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context->Esp;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context->Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
#endif

    IMAGEHLP_LINE64 line = {0};
    line.SizeOfStruct = sizeof line;
    const auto imageType = ImageNtHeader(modules[0].baseAddress)->FileHeader.Machine;

    std::ostringstream stacktrace;

    const auto currentThread = GetCurrentThread();
    DWORD offset_from_symbol = 0;
    int stackLineId = 0;
    do {
        if (frame.AddrPC.Offset != 0) {
            std::string fnName = symbol(currentProcess, frame.AddrPC.Offset).unDecoratedNme();
            if (fnName != "RaiseException") {
                stacktrace << "[" << stackLineId << "] " << fnName;

                if (SymGetLineFromAddr64(currentProcess, frame.AddrPC.Offset, &offset_from_symbol, &line)) {
                    stacktrace << ", " << line.FileName << ":" << line.LineNumber << ")";
                }
                stacktrace << "\n";

                if (fnName == "main") {
                    break;
                }

                ++stackLineId;
            }
        }

        if (!StackWalk64(imageType,
                         currentProcess,
                         currentThread,
                         &frame,
                         context,
                         NULL,
                         SymFunctionTableAccess64,
                         SymGetModuleBase64,
                         NULL)) {
            break;
        }
    } while (frame.AddrReturn.Offset != 0);

    SymCleanup(currentProcess);

    return stacktrace.str();
}

const std::string createMiniDump(EXCEPTION_POINTERS *exceptionPtr)
{
    const auto currentProcess = GetCurrentProcess();
    const auto currentProcessId = GetCurrentProcessId();
    const auto currentThreadId = GetCurrentThreadId();

    LOG_INFO_STATIC("currentProcessId: " << currentProcessId << ", currentThreadId: " << currentThreadId);

    // Load dbghelp.dll
    auto hDbgHelp = LoadLibrary(_T("dbghelp.dll"));
    if (hDbgHelp == nullptr) {
        LOG_ERROR_STATIC("Couldn't load dbghelp.dll");
        return "";
    }

    const std::string fileName = "crashdump" + tools::generateTimeStamp() + "_" + std::to_string(currentProcessId) + "_"
                                 + std::to_string(currentThreadId) + ".dmp";
    HANDLE hFile =
        CreateFile(fileName.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

    MINIDUMP_EXCEPTION_INFORMATION excInfo;
    excInfo.ThreadId = currentThreadId;
    excInfo.ExceptionPointers = exceptionPtr;
    excInfo.ClientPointers = TRUE;

    const bool bWriteDump =
        MiniDumpWriteDump(currentProcess, currentProcessId, hFile, MiniDumpWithDataSegs, &excInfo, NULL, NULL);
    if (!bWriteDump) {
        LOG_ERROR_STATIC("Error writing dump: " << fileName);
        CloseHandle(hFile);
        FreeLibrary(hDbgHelp);
        return "";
    }

    CloseHandle(hFile);
    FreeLibrary(hDbgHelp);

    LOG_INFO_STATIC("Success!");

    return fileName;
}

LONG WINAPI unhandledExceptionsHandler(PEXCEPTION_POINTERS exceptionPtr, std::string &error, std::string &stacktrace)
{
    const auto fileName = createMiniDump(exceptionPtr);

    std::ostringstream ss;
    ss << convertExceptionCodeToString(exceptionPtr->ExceptionRecord->ExceptionCode);
    ss << " (code: 0x" << tools::to_hex_string(exceptionPtr->ExceptionRecord->ExceptionCode);
    ss << " flags: 0x" << tools::to_hex_string(exceptionPtr->ExceptionRecord->ExceptionFlags) << ")";
    error = ss.str();

    stacktrace = createStacktrace(exceptionPtr);

    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

CrashHandler::CrashHandler()
{
#ifdef __linux__
    struct rlimit core_limits;
    core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &core_limits);

    const std::string cmd("sysctl -w kernel.core_pattern=");
    const std::string folder(CRASHDUMPS_DIR);
    const std::string fileName("core-%e.%p.%h.%t");
    executeCommand(cmd + folder + "/" + fileName);
#endif
}

CrashHandler::CrashEvent::Interface &CrashHandler::crashEvent()
{
    return m_crashEvent;
}

void CrashHandler::handleException(std::string &stacktrace)
{
#ifdef __linux__
    const size_t STACK_TRACE_SZ = 100;
    void *buffer[STACK_TRACE_SZ];
    const size_t stacktraceSz = backtrace(buffer, STACK_TRACE_SZ);
    auto stacktrace = backtrace_symbols(buffer, stacktraceSz);

    std::ostringstream ss;
    ss << "stacktrace:" << std::endl;
    for (size_t i = 1; i < stacktraceSz; ++i) {
        ss << "[" << i << "] " << stacktrace[i] << std::endl;
    }

    stacktrace = ss.str();
#else
    stacktrace = stacktrace;
#endif
}

void CrashHandler::handleSignals()
{
    auto handler = [](int signal) {
        const std::string error = "signal: " + std::to_string(signal);
        throw std::domain_error(error);
    };
    signal(SIGINT, handler);
    signal(SIGSEGV, handler);
    // signal(SIGFPE, handler);
}

void invokeC(CrashHandler::Func &&fn, std::string &error, std::string &stacktrace)
{
#ifdef __linux__
    fn();
    error = "No error";
    stacktrace = "No stacktrace";
#elif _WIN32
    __try {
        fn();
    } __except (unhandledExceptionsHandler(GetExceptionInformation(), error, stacktrace)) {
        terminate();
    }
#endif
}

void CrashHandler::invokeImpl(Func &&fn)
{
    std::string error = "Unknown";
    std::string stacktrace = "No stacktrace";

    try {
        handleSignals();
        invokeC(std::move(fn), error, stacktrace);
    } catch (const std::domain_error &ex) {
        handleException(stacktrace);
        m_crashEvent.notify(ex.what(), stacktrace);
    } catch (const std::logic_error &ex) {
        handleException(stacktrace);
        m_crashEvent.notify(ex.what(), stacktrace);
    } catch (const std::runtime_error &ex) {
        handleException(stacktrace);
        m_crashEvent.notify(ex.what(), stacktrace);
    } catch (...) {
        handleException(stacktrace);
        m_crashEvent.notify(error, stacktrace);
    }
}

void CrashHandler::invoke(Func &&fn)
{
#ifdef __linux__
    invokeImpl(std::move(fn));
#elif _WIN32
    __try {
        invokeImpl(std::move(fn));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
#endif
}

} // namespace psi::thread
