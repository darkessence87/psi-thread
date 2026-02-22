
#pragma once

#include <string>

namespace psi::thread {

std::string createStacktrace();
std::string createStacktrace(const CONTEXT& ctx);

} // namespace psi::thread
