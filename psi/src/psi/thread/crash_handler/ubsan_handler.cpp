
#ifdef __linux__
#include <execinfo.h>
#include <sys/resource.h>
#elif _WIN32
#include <Windows.h>

#include <DbgHelp.h>
#endif

#include <functional>
#include <string>

#include "crash_notifier.h"
#include "mini_dump.h"
#include "stacktrace.h"
#include "ubsan_handler.h"

namespace psi::thread {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
[[noreturn]] static void handle_ubsan(const char *kind) noexcept
{
    createMiniDump(nullptr, nullptr);

    if (t_crash_notifier) {
        std::string st = createStacktrace();
        (*t_crash_notifier)(kind, st.c_str());
    }

    ExitThread(0);
}

extern "C" {
__declspec(dllexport) void __ubsan_handle_type_mismatch_v1(void *, void *)
{
    handle_ubsan("UBSan type_mismatch_v1");
}

__declspec(dllexport) void __ubsan_handle_type_mismatch_v1_abort(void *, void *)
{
    handle_ubsan("UBSan type_mismatch_v1");
}

__declspec(dllexport) void __ubsan_handle_divrem_overflow(void *, void *)
{
    handle_ubsan("UBSan divrem_overflow");
}

__declspec(dllexport) void __ubsan_handle_divrem_overflow_abort(void *, void *)
{
    handle_ubsan("UBSan divrem_overflow");
}
}

#pragma clang diagnostic pop

} // namespace psi::thread
