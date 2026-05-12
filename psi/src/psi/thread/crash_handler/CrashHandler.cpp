#include "psi/thread/CrashHandler.h"
#include "psi/tools/Tools.h"

#ifdef __linux__
#include <cstdio>
#include <execinfo.h>
#include <sys/resource.h>
#ifndef CRASHDUMPS_DIR
#define CRASHDUMPS_DIR "/tmp"
#endif
#endif

#include <array>
#include <functional>
#include <memory>
#include <signal.h>
#include <sstream>
#include <string>
#include <utility>

#include "crash_notifier.h"
#include "stacktrace.h"
#ifdef _WIN32
#include "dbg_helper.h"
#include "mini_dump.h"
#include "seh_handler.h"
#endif

namespace psi::thread {

#ifdef __linux__
std::string executeCommand(const std::string &cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}
#endif

thread_local CrashNotifyFn *t_crash_notifier = nullptr;

#ifdef _WIN32
static std::string convertExceptionCodeToString(DWORD exCode)
{
    LPSTR buffer = nullptr;

    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

    DWORD len = FormatMessageA(
        flags, nullptr, exCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&buffer), 0, nullptr);

    if (len == 0 || buffer == nullptr) {
        return "Unknown exception";
    }

    std::string result(buffer, len);
    LocalFree(buffer);

    return tools::trim(result);
}
#endif

static void invokeC(CrashHandler::Func &&fn, std::string &error, std::string &stacktrace)
{
#ifdef __linux__
    fn();
    error = "No error";
    stacktrace = "No stacktrace";
#elif _WIN32
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
    __try {
#pragma clang diagnostic pop
        seh_handler_init();
        fn();
    } __except (seh_filter(GetExceptionInformation())) {
        auto ep = seh_handler_get_exception();
        if (!ep) {
            error = "No SEH exception context";
            stacktrace = "Unavailable";
            seh_handler_release();
            return;
        }

        auto record = *ep->ExceptionRecord;
        auto context = *ep->ContextRecord;

        createMiniDump(&record, &context);

        auto code = record.ExceptionCode;
        auto flags = record.ExceptionFlags;

        std::ostringstream ss;
        ss << convertExceptionCodeToString(code);
        ss << " (code: 0x" << tools::to_hex_string(code);
        ss << " flags: 0x" << tools::to_hex_string(flags) << ")";
        error = ss.str();

        stacktrace = createStacktrace(context);

        (*t_crash_notifier)(error.c_str(), stacktrace.c_str());
        seh_handler_release();
    }
#endif
}

CrashHandler::CrashHandler()
{
#ifdef __linux__
    struct rlimit core_limits;
    core_limits.rlim_cur = core_limits.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &core_limits);

    const std::string cmd("sysctl -w kernel.core_pattern=");
    const std::string folder(CRASHDUMPS_DIR);
    const std::string fileName("core-%e.%p.%h.%t");
    executeCommand(cmd + folder + "/" + fileName);
#elif _WIN32
    dbg_helper::instance();
#endif
}

CrashHandler::CrashEvent::Interface &CrashHandler::crashEvent()
{
    return m_crashEvent;
}

void CrashHandler::handleException([[maybe_unused]] std::string &st)
{
#ifdef __linux__
    const size_t STACK_TRACE_SZ = 100;
    void *buffer[STACK_TRACE_SZ];
    const size_t stacktraceSz = backtrace(buffer, STACK_TRACE_SZ);
    auto stacktrace = backtrace_symbols(buffer, stacktraceSz);

    std::ostringstream ss;
    ss << "stacktrace:" << std::endl;
    for (size_t i = 1; i < stacktraceSz; ++i) {
        ss << "[" << i << "] " << stacktrace[i] << std::endl;
    }

    st = ss.str();
#endif
}

void CrashHandler::handleSignals()
{
    auto handler = [](int signal) {
        const std::string error = "signal: " + std::to_string(signal);
        throw std::domain_error(error);
    };
    signal(SIGINT, handler);
    signal(SIGSEGV, handler);
    // signal(SIGFPE, handler);
}

void CrashHandler::invokeImpl(Func &&fn)
{
    CrashNotifyFn crash_notifier = [this](const char *err, const char *st) { m_crashEvent.notify(err, st); };
    t_crash_notifier = &crash_notifier;

    std::string error = "Unknown";
    std::string stacktrace = "No stacktrace";

    try {
        handleSignals();
        invokeC(std::move(fn), error, stacktrace);
    } catch (const std::domain_error &ex) {
        handleException(stacktrace);
        m_crashEvent.notify(ex.what(), stacktrace);
    } catch (const std::logic_error &ex) {
        handleException(stacktrace);
        m_crashEvent.notify(ex.what(), stacktrace);
    } catch (const std::runtime_error &ex) {
        handleException(stacktrace);
        m_crashEvent.notify(ex.what(), stacktrace);
    } catch (...) {
        handleException(stacktrace);
        m_crashEvent.notify(error, stacktrace);
    }

    t_crash_notifier = nullptr;
}

void CrashHandler::invoke(Func &&fn)
{
    invokeImpl(std::move(fn));
}

} // namespace psi::thread
