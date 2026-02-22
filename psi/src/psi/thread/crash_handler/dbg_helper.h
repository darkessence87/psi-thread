#pragma once

#if _WIN32
#include <Windows.h>

#include <DbgHelp.h>
#include <string>

namespace psi::thread {

class dbg_helper
{
public:
    static dbg_helper &instance();

    bool available() const noexcept;

    bool symInitialize();
    void symCleanup();

    bool stackWalk64(DWORD machine, HANDLE thread, LPSTACKFRAME64 frame, PVOID context);
    bool symFromAddr(DWORD64 addr, DWORD64 *displacement, PSYMBOL_INFO sym);
    bool symGetLineFromAddr64(DWORD64 addr, DWORD *displacement, PIMAGEHLP_LINE64 line);
    PVOID symFunctionTableAccess64(DWORD64 addr);
    DWORD64 symGetModuleBase64(DWORD64 addr);
    bool miniDumpWriteDump(DWORD pid, HANDLE file, MINIDUMP_TYPE type, MINIDUMP_EXCEPTION_INFORMATION *exc);

private:
    dbg_helper();

    dbg_helper(const dbg_helper &) = delete;
    dbg_helper &operator=(const dbg_helper &) = delete;

    void load();

    HMODULE m_module = nullptr;
    HANDLE m_process = nullptr;
    bool m_loaded = false;

    // function pointers
    decltype(&MiniDumpWriteDump) pMiniDumpWriteDump = nullptr;
    decltype(&StackWalk64) pStackWalk64 = nullptr;
    decltype(&SymInitialize) pSymInitialize = nullptr;
    decltype(&SymCleanup) pSymCleanup = nullptr;
    decltype(&SymFromAddr) pSymFromAddr = nullptr;
    decltype(&SymGetLineFromAddr64) pSymGetLineFromAddr64 = nullptr;
    decltype(&SymFunctionTableAccess64) pSymFunctionTableAccess64 = nullptr;
    decltype(&SymGetModuleBase64) pSymGetModuleBase64 = nullptr;
    decltype(&SymSetOptions) pSymSetOptions = nullptr;
    decltype(&SymGetOptions) pSymGetOptions = nullptr;
};

} // namespace psi::thread

#endif
