
#include "stacktrace.h"

#ifdef __linux__
#include <execinfo.h>
#include <sys/resource.h>
#elif _WIN32
#include <Windows.h>
#endif

#include <algorithm>
#include <array>
#include <vector>

#include "psi/tools/Tools.h"
#include "dbg_helper.h"

namespace psi::thread {

class symbol
{
    IMAGEHLP_SYMBOL64 *sym;
    const uint32_t maxNameLen = 1024u;

public:
    symbol(HANDLE process, DWORD64 address)
    {
        const size_t sz = sizeof(IMAGEHLP_SYMBOL64) + maxNameLen;
        sym = static_cast<IMAGEHLP_SYMBOL64 *>(malloc(sz));
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
        std::memset(reinterpret_cast<uint8_t *>(sym), uint8_t('\0'), sz);
#pragma clang diagnostic pop
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
        DWORD len = UnDecorateSymbolName(sym->Name, undName.data(), static_cast<DWORD>(undName.size()), UNDNAME_COMPLETE);
        if (len == 0) {
            return sym->Name;
        }

        return std::string(undName.data(), len);
    }
};

std::string createStacktrace()
{
#ifdef _WIN32
    auto &dbg = dbg_helper::instance();
    if (!dbg.available()) {
        return "Unavailable";
    }

    constexpr USHORT MAX_FRAMES = 64;
    std::array<void *, MAX_FRAMES> frames;

    USHORT count = CaptureStackBackTrace(3, MAX_FRAMES, frames.data(), nullptr);
    if (count == 0) {
        return "No stacktrace";
    }

    dbg.symInitialize();

    std::ostringstream out;

    for (USHORT i = 0; i < count; ++i) {
        DWORD64 addr = reinterpret_cast<DWORD64>(frames[i]);

        char buffer[sizeof(SYMBOL_INFO) + 1024];
        auto *sym = reinterpret_cast<SYMBOL_INFO *>(buffer);
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 1024;

        DWORD64 displacement = 0;

        if (dbg.symFromAddr(addr, &displacement, sym)) {
            out << "[" << i << "] " << sym->Name;

            IMAGEHLP_LINE64 line {};
            line.SizeOfStruct = sizeof(line);
            DWORD lineDisp = 0;

            if (dbg.symGetLineFromAddr64(addr, &lineDisp, &line)) {
                out << ", " << line.FileName << ":" << line.LineNumber;
            }

            out << "\n";
        } else {
            out << "[" << i << "] 0x" << std::hex << addr << std::dec << "\n";
        }
    }

    dbg.symCleanup();
    return out.str();
#else
    return "Not implemented";
#endif
}

std::string createStacktrace(const CONTEXT &ctx)
{
    auto &dbg = dbg_helper::instance();
    if (!dbg.available()) {
        return "Unavailable";
    }

    HANDLE thread = GetCurrentThread();

    STACKFRAME64 frame {};
#ifdef _M_X64
    frame.AddrPC.Offset = ctx.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
    DWORD imageType = IMAGE_FILE_MACHINE_AMD64;
#else
    frame.AddrPC.Offset = ctx.Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Esp;
    frame.AddrStack.Mode = AddrModeFlat;
    DWORD imageType = IMAGE_FILE_MACHINE_I386;
#endif

    CONTEXT walkCtx = ctx;

    std::ostringstream out;

    for (int i = 0; i < 64; ++i) {
        if (!dbg.stackWalk64(imageType, thread, &frame, &walkCtx)) {
            break;
        }

        DWORD64 addr = frame.AddrPC.Offset;
        if (addr == 0) {
            break;
        }

        char buffer[sizeof(SYMBOL_INFO) + 1024];
        auto *sym = reinterpret_cast<SYMBOL_INFO *>(buffer);
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 1024;

        DWORD64 displacement = 0;

        if (dbg.symFromAddr(addr, &displacement, sym)) {
            out << "[" << i << "] " << sym->Name;

            IMAGEHLP_LINE64 line {};
            line.SizeOfStruct = sizeof(line);
            DWORD lineDisp = 0;

            if (dbg.symGetLineFromAddr64(addr, &lineDisp, &line)) {
                out << ", " << line.FileName << ":" << line.LineNumber;
            }

            out << "\n";
        } else {
            out << "[" << i << "] 0x" << std::hex << addr << std::dec << "\n";
        }
    }

    return out.str();
}

} // namespace psi::thread
