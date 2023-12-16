#include "psi/thread/ThreadPool.h"

#include "psi/thread/CrashHandler.h"

#ifdef PSI_LOGGER
#include "psi/logger/Logger.h"
#else
#include <iostream>
#include <sstream>
#define LOG_INFO(x)                                                                                                    \
    do {                                                                                                               \
        std::ostringstream os;                                                                                         \
        os << x;                                                                                                       \
        std::cout << os.str() << std::endl;                                                                            \
    } while (0)
#define LOG_ERROR(x) LOG_INFO(x)
#endif

namespace psi::thread {

ThreadPool::ThreadPool(uint8_t numberOfThreads)
    : m_isActive(false)
    , m_maxNumberOfThreads(numberOfThreads)
{
}

ThreadPool::~ThreadPool()
{
    interrupt();
}

void ThreadPool::run()
{
    if (m_isActive) {
        return;
    }

    m_isActive = true;

    for (uint8_t i = 0; i < m_maxNumberOfThreads; ++i) {
        m_threads.emplace_back(std::thread(std::bind(&ThreadPool::onThreadUpdate, this)));
    }

    while (m_aliveThreads < m_maxNumberOfThreads) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void ThreadPool::join()
{
    for (auto &thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void ThreadPool::interrupt()
{
    if (m_isActive) {
        m_isActive = false;
        m_condition.notify_all();
    }

    join();

    m_threads.clear();
}

void ThreadPool::onThreadUpdate()
{
    const auto threadId = std::this_thread::get_id();
    LOG_INFO("Start pool thread: " << threadId);

    auto runThread = [this]() {
        ++m_aliveThreads;

        while (m_isActive) {
            trigger();
        }

        while (!m_queue.empty()) {
            trigger();
        }
    };

    CrashHandler ch;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_onCrashSubs[threadId] = ch.crashEvent().subscribe([this](const auto &error, const auto &stacktrace) {
            LOG_ERROR("Crash in pool thread: " << std::this_thread::get_id() << ", error: [" << error << "]");
            LOG_ERROR(stacktrace);
        });
    }

    ch.invoke(runThread);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_onCrashSubs.erase(m_onCrashSubs.find(threadId));
    }

    --m_aliveThreads;

    LOG_INFO("Exit pool thread: " << threadId);
}

size_t ThreadPool::getWorkload() const
{
    return m_queue.size();
}

void ThreadPool::invoke(Func &&fn)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    m_queue.emplace(std::forward<Func>(fn));
    m_condition.notify_one();
}

bool ThreadPool::isRunning()
{
    return m_isActive;
}

void ThreadPool::trigger()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_aliveThreads) {
        m_condition.wait(lock, [this]() { return !m_queue.empty() || !m_isActive; });
    }

    if (m_queue.empty()) {
        return;
    }

    auto fn = m_queue.front();
    m_queue.pop();

    lock.unlock();

    fn();
}

} // namespace psi