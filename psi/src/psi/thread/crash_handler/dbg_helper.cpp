#if _WIN32

#include "dbg_helper.h"

#include <bit>

namespace psi::thread {

namespace {
template <typename T>
T load_proc(HMODULE m, const char *name)
{
    return std::bit_cast<T>(GetProcAddress(m, name));
}
} // namespace

dbg_helper &dbg_helper::instance()
{
    static dbg_helper inst;
    return inst;
}

dbg_helper::dbg_helper()
{
    load();
}

void dbg_helper::load()
{
    m_module = LoadLibraryA("DbgHelp.dll");
    if (!m_module) {
        return;
    }

    m_process = GetCurrentProcess();

    pMiniDumpWriteDump = load_proc<decltype(pMiniDumpWriteDump)>(m_module, "MiniDumpWriteDump");
    pStackWalk64 = load_proc<decltype(pStackWalk64)>(m_module, "StackWalk64");
    pSymInitialize = load_proc<decltype(pSymInitialize)>(m_module, "SymInitialize");
    pSymCleanup = load_proc<decltype(pSymCleanup)>(m_module, "SymCleanup");
    pSymFromAddr = load_proc<decltype(pSymFromAddr)>(m_module, "SymFromAddr");
    pSymGetLineFromAddr64 = load_proc<decltype(pSymGetLineFromAddr64)>(m_module, "SymGetLineFromAddr64");
    pSymFunctionTableAccess64 = load_proc<decltype(pSymFunctionTableAccess64)>(m_module, "SymFunctionTableAccess64");
    pSymGetModuleBase64 = load_proc<decltype(pSymGetModuleBase64)>(m_module, "SymGetModuleBase64");
    pSymSetOptions = load_proc<decltype(pSymSetOptions)>(m_module, "SymSetOptions");
    pSymGetOptions = load_proc<decltype(pSymGetOptions)>(m_module, "SymGetOptions");

    m_loaded = pMiniDumpWriteDump && pStackWalk64 && pSymInitialize;
}

bool dbg_helper::available() const noexcept
{
    return m_loaded;
}

bool dbg_helper::symInitialize()
{
    if (!m_loaded) {
        return false;
    }

    pSymInitialize(GetCurrentProcess(), nullptr, TRUE);

    DWORD opts = pSymGetOptions();
    opts |= SYMOPT_LOAD_LINES | SYMOPT_UNDNAME;
    pSymSetOptions(opts);

    return true;
}

void dbg_helper::symCleanup()
{
    if (m_loaded) {
        pSymCleanup(GetCurrentProcess());
    }
}

bool dbg_helper::stackWalk64(DWORD machine, HANDLE thread, LPSTACKFRAME64 frame, PVOID context)
{
    return pStackWalk64
           && pStackWalk64(machine, m_process, thread, frame, context, nullptr, pSymFunctionTableAccess64, pSymGetModuleBase64, nullptr);
}

bool dbg_helper::symFromAddr(DWORD64 addr, DWORD64 *displacement, PSYMBOL_INFO sym)
{
    return pSymFromAddr && pSymFromAddr(m_process, addr, displacement, sym);
}

bool dbg_helper::symGetLineFromAddr64(DWORD64 addr, DWORD *displacement, PIMAGEHLP_LINE64 line)
{
    return pSymGetLineFromAddr64 && pSymGetLineFromAddr64(m_process, addr, displacement, line);
}

PVOID dbg_helper::symFunctionTableAccess64(DWORD64 addr)
{
    return pSymFunctionTableAccess64 ? pSymFunctionTableAccess64(m_process, addr) : nullptr;
}

DWORD64 dbg_helper::symGetModuleBase64(DWORD64 addr)
{
    return pSymGetModuleBase64 ? pSymGetModuleBase64(m_process, addr) : 0;
}

bool dbg_helper::miniDumpWriteDump(DWORD pid,
                                   HANDLE file,
                                   MINIDUMP_TYPE type,
                                   MINIDUMP_EXCEPTION_INFORMATION *exc)
{
    return pMiniDumpWriteDump && pMiniDumpWriteDump(m_process, pid, file, type, exc, nullptr, nullptr);
}

} // namespace psi::thread

#endif
