
#include "seh_handler.h"

#include <sstream>

#include "psi/tools/Tools.h"
#include "stacktrace.h"

namespace psi::thread {

struct SehContext {
    PEXCEPTION_POINTERS m_exc_ptrs = {};
    bool m_is_triggerred = false;
};

static thread_local SehContext t_seh_context;

void seh_handler_init()
{
    t_seh_context.m_exc_ptrs = nullptr;
    t_seh_context.m_is_triggerred = false;
}

PEXCEPTION_POINTERS seh_handler_get_exception()
{
    auto ep = t_seh_context.m_exc_ptrs;

    if (!t_seh_context.m_is_triggerred || !ep || !ep->ExceptionRecord) {
        return nullptr;
    }

    return ep;
}

void seh_handler_release()
{
    t_seh_context.m_exc_ptrs = nullptr;
    t_seh_context.m_is_triggerred = false;
}

LONG WINAPI seh_filter(PEXCEPTION_POINTERS ep)
{
    if (t_seh_context.m_is_triggerred && t_seh_context.m_exc_ptrs) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    if (!ep || !ep->ExceptionRecord) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    const DWORD code = ep->ExceptionRecord->ExceptionCode;

    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_STACK_OVERFLOW:
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    case EXCEPTION_DATATYPE_MISALIGNMENT:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        t_seh_context.m_exc_ptrs = ep;
        t_seh_context.m_is_triggerred = true;
        return EXCEPTION_EXECUTE_HANDLER;

    default:
        return EXCEPTION_CONTINUE_SEARCH;
    }
}

} // namespace psi::thread
