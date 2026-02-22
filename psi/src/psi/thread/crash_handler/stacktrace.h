
#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

#include <string>

namespace psi::thread {

std::string createStacktrace();
std::string createStacktrace(const CONTEXT& ctx);

} // namespace psi::thread
