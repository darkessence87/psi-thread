
#include "mini_dump.h"

#if _WIN32
#include <DbgHelp.h>
#endif

#include <bit>
#include <sstream>
#include <string>

#include "psi/tools/Tools.h"

#ifdef PSI_LOGGER
#include "psi/logger/Logger.h"
#else
#include <iostream>
#define LOG_INFO_STATIC(x)                                                                                             \
    do {                                                                                                               \
        std::ostringstream os;                                                                                         \
        os << x;                                                                                                       \
        std::cout << os.str() << std::endl;                                                                            \
    } while (0)
#define LOG_INFO_ERROR(x) LOG_INFO_STATIC(x)
#endif

namespace psi::thread {

using MiniDumpWriteDump_t = BOOL(WINAPI *)(HANDLE,
                                           DWORD,
                                           HANDLE,
                                           MINIDUMP_TYPE,
                                           PMINIDUMP_EXCEPTION_INFORMATION,
                                           PMINIDUMP_USER_STREAM_INFORMATION,
                                           PMINIDUMP_CALLBACK_INFORMATION);
static MiniDumpWriteDump_t pMiniDumpWriteDump = nullptr;

static void init_dbghelp_dynamic()
{
    static bool initialized = false;
    if (initialized) {
        return;
    }

    HMODULE hDbgHelp = LoadLibraryA("DbgHelp.dll");
    if (!hDbgHelp) {
        return;
    }

    auto proc = GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
    pMiniDumpWriteDump = std::bit_cast<MiniDumpWriteDump_t>(proc);

    initialized = true;
}

void createMiniDump(const EXCEPTION_RECORD *record, const CONTEXT *context)
{
    init_dbghelp_dynamic();

    const DWORD pid = GetCurrentProcessId();
    const DWORD tid = GetCurrentThreadId();

    LOG_INFO_STATIC("currentProcessId: " << pid << ", currentThreadId: " << tid);

    const std::string fileName =
        "crashdump" + tools::generateTimeStamp() + "_" + std::to_string(pid) + "_" + std::to_string(tid) + ".dmp";

    HANDLE hFile = CreateFileA(fileName.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }

    MINIDUMP_EXCEPTION_INFORMATION excInfo {};
    MINIDUMP_EXCEPTION_INFORMATION *excInfoPtr = nullptr;

    EXCEPTION_POINTERS ep {};
    if (record && context) {
        ep.ExceptionRecord = const_cast<EXCEPTION_RECORD *>(record);
        ep.ContextRecord = const_cast<CONTEXT *>(context);

        excInfo.ThreadId = tid;
        excInfo.ExceptionPointers = &ep;
        excInfo.ClientPointers = FALSE;

        excInfoPtr = &excInfo;
    }

    BOOL is_ok =
        MiniDumpWriteDump(GetCurrentProcess(),
                          pid,
                          hFile,
                          static_cast<MINIDUMP_TYPE>(MiniDumpWithFullMemory | MiniDumpWithHandleData | MiniDumpWithThreadInfo
                                                     | MiniDumpWithUnloadedModules | MiniDumpWithFullMemoryInfo),
                          excInfoPtr,
                          nullptr,
                          nullptr);
    if (!is_ok) {
        CloseHandle(hFile);

        LOG_INFO_ERROR("Failed! " << fileName);
        return;
    }

    CloseHandle(hFile);

    LOG_INFO_STATIC("Success! " << fileName);
}

} // namespace psi::thread
