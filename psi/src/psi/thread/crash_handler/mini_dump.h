
#pragma once

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>

namespace psi::thread {

void createMiniDump(const EXCEPTION_RECORD *record, const CONTEXT *context);

} // namespace psi::thread

#endif
