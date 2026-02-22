
#include "mini_dump.h"

#include <sstream>
#include <string>

#include "dbg_helper.h"
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

void createMiniDump(const EXCEPTION_RECORD *record, const CONTEXT *context)
{
    auto &dbg = dbg_helper::instance();
    if (!dbg.available()) {
        return;
    }

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

    BOOL is_ok = dbg.miniDumpWriteDump(pid,
                                       hFile,
                                       static_cast<MINIDUMP_TYPE>(MiniDumpWithFullMemory | MiniDumpWithHandleData
                                                                  | MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules
                                                                  | MiniDumpWithFullMemoryInfo),
                                       excInfoPtr);
    if (!is_ok) {
        CloseHandle(hFile);

        LOG_INFO_ERROR("Failed! " << fileName);
        return;
    }

    CloseHandle(hFile);

    LOG_INFO_STATIC("Success! " << fileName);
}

} // namespace psi::thread
