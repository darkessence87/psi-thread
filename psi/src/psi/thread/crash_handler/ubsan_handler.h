
#pragma once

namespace psi::thread {

extern "C" {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
[[noreturn]] __declspec(dllexport) void __ubsan_handle_type_mismatch_v1(void *, void *);
[[noreturn]] __declspec(dllexport) void __ubsan_handle_type_mismatch_v1_abort(void *, void *);
[[noreturn]] __declspec(dllexport) void __ubsan_handle_divrem_overflow(void *, void *);
[[noreturn]] __declspec(dllexport) void __ubsan_handle_divrem_overflow_abort(void *, void *);
#pragma clang diagnostic pop
}

} // namespace psi::thread
