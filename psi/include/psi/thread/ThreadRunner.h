#pragma once

#include <functional>
#include <thread>

#include "CrashHandler.h"

#include "psi/logger/Logger.h"

class ThreadRunner
{
public:
    using RunFn = std::function<void()>;
    using OnCrashFn = std::function<void()>;

    ThreadRunner(const std::string &threadName, RunFn &&runFn)
        : m_threadName(threadName)
        , m_runFn(std::forward<RunFn>(runFn))
    {
    }

    ~ThreadRunner()
    {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    void run(OnCrashFn &&onCrashFn)
    {
        if (m_thread.joinable()) {
            m_thread.join();
        }

        m_thread = std::thread([=, this]() {
            m_onCrashSub =
                m_crashHandler.crashEvent().subscribe([this, onCrashFn](const auto &errorMsg, const auto &stacktrace) {
                    LOG_ERROR("Crash in " << m_threadName << " thread, [" << errorMsg << "]");
                    LOG_ERROR(stacktrace);
                    onCrashFn();
                });

            m_crashHandler.invoke([this]() {
                LOG_INFO("Run " << m_threadName);
                m_runFn();
            });

            m_onCrashSub.reset();
        });
    }

private:
    const std::string m_threadName;
    std::thread m_thread;
    psi::thread::CrashHandler m_crashHandler;
    psi::comm::Subscription m_onCrashSub;
    RunFn m_runFn;
};