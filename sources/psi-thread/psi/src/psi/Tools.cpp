// #define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <codecvt>
#include <ctime>

#include "psi/Tools.h"

namespace psi::tools {

std::wstring utf8_to_wstring(const std::string &str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.from_bytes(str);
}

std::string wstring_to_utf8(const std::wstring &str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(str);
}

std::string generateTimeStamp() noexcept
{
    using namespace std::chrono;

    std::stringstream ss;
#ifdef __linux__
    const auto curTime = system_clock::now();
    const auto time = system_clock::to_time_t(curTime);
    const auto localTime = std::localtime(&time);
    ss << std::put_time(localTime, "%Y.%m.%d_%H.%M.%S");
#elif _WIN32
    const auto curTime = system_clock::now();
    struct tm buf;
    const auto time = system_clock::to_time_t(curTime);
    localtime_s(&buf, &time);
    ss << std::put_time(&buf, "%Y.%m.%d_%H.%M.%S");
#endif
    return ss.str();
}

} // namespace psi::tools
