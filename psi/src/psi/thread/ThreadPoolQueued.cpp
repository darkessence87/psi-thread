
#include "psi/thread/ThreadPoolQueued.h"
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

ThreadPoolQueued::SimpleThread::SimpleThread()
    : m_isActive(false)
{
}

ThreadPoolQueued::SimpleThread::~SimpleThread()
{
    interrupt();
}

void ThreadPoolQueued::SimpleThread::run()
{
    if (m_isActive) {
        return;
    }

    m_isActive = true;

    m_thread = std::thread(std::bind(&ThreadPoolQueued::SimpleThread::onThreadUpdate, this));
}

void ThreadPoolQueued::SimpleThread::interrupt()
{
    if (m_isActive) {
        m_isActive = false;
        m_condition.notify_all();
    }

    join();
}

void ThreadPoolQueued::SimpleThread::join()
{
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

bool ThreadPoolQueued::SimpleThread::isRunning()
{
    return m_isActive;
}

size_t ThreadPoolQueued::SimpleThread::getWorkload() const
{
    return m_queue.size();
}

void ThreadPoolQueued::SimpleThread::invoke(Func &&fn)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue.emplace(fn);
    m_condition.notify_one();
}

void ThreadPoolQueued::SimpleThread::onThreadUpdate()
{
    LOG_INFO("Start pool queued thread:" << std::this_thread::get_id());

    psi::thread::CrashHandler ch;
    m_crashSub = ch.crashEvent().subscribe([this](const auto &error, const auto &) {
        LOG_ERROR("Crash in pool queued thread:" << std::this_thread::get_id() << ", error: " << error);
    });
    ch.invoke([this]() {
        while (m_isActive) {
            trigger();
        }

        while (!m_queue.empty()) {
            trigger();
        }
    });

    m_isActive = false;
    m_crashSub.reset();

    LOG_INFO("Exit pool queued thread:" << std::this_thread::get_id());
}

void ThreadPoolQueued::SimpleThread::trigger()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this]() { return !m_queue.empty() || !m_isActive; });

    if (m_queue.empty()) {
        return;
    }

    auto fn = m_queue.front();
    m_queue.pop();

    lock.unlock();

    fn();
}

ThreadPoolQueued::ThreadPoolQueued(uint8_t numberOfThreads)
    : m_threadIndex(0)
    , m_maxNumberOfThreads(numberOfThreads)
{
}

ThreadPoolQueued::~ThreadPoolQueued()
{
    interrupt();
}

void ThreadPoolQueued::run()
{
    for (uint8_t i = 0; i < m_maxNumberOfThreads; ++i) {
        m_threads.emplace_back(std::make_unique<SimpleThread>());
    }

    for (auto &t : m_threads) {
        t->run();
    }
}

void ThreadPoolQueued::interrupt()
{
    for (auto &t : m_threads) {
        t->interrupt();
    }
}

bool ThreadPoolQueued::isRunning()
{
    for (auto &t : m_threads) {
        if (t->isRunning()) {
            return true;
        }
    }

    return false;
}

void ThreadPoolQueued::trigger()
{
    for (auto &t : m_threads) {
        t->trigger();
    }
}

void ThreadPoolQueued::invoke(Func &&fn)
{
    const size_t index = m_threadIndex++ % m_threads.size();
    auto &t = m_threads[index];
    if (t->isRunning()) {
        t->invoke(std::move(fn));
    } else if (isRunning()) {
        invoke(std::move(fn));
    }
}

size_t ThreadPoolQueued::getWorkload() const
{
    size_t result = 0u;

    for (auto &t : m_threads) {
        result += t->getWorkload();
    }

    return result;
}

void ThreadPoolQueued::join()
{
    for (auto &t : m_threads) {
        t->join();
    }
}

} // namespace psi::thread