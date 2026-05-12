
#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

#include <string>

namespace psi::thread {

std::string createStacktrace();
#ifdef _WIN32
std::string createStacktrace(const CONTEXT& ctx);
#endif

} // namespace psi::thread
