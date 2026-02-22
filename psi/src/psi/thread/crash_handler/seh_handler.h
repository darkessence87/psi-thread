
#pragma once

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

namespace psi::thread {

void seh_handler_init();
PEXCEPTION_POINTERS seh_handler_get_exception();
void seh_handler_release();
LONG WINAPI seh_filter(PEXCEPTION_POINTERS ep);

} // namespace psi::thread

#endif
