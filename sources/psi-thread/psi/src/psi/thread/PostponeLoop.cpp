
#include "psi/thread/PostponeLoop.h"
#include "psi/thread/CrashHandler.h"

#ifdef PSI_EXAMPLE
#include <iostream>
#include <sstream>
#define PSI_EXAMPLE_LOG(x)                                                                                             \
    do {                                                                                                               \
        std::ostringstream os;                                                                                         \
        os << x;                                                                                                       \
        std::cout << os.str() << std::endl;                                                                            \
    } while (0)
#else
#error "Provide your own logger"
#endif

namespace psi::thread {

PostponeLoop::PostponeLoop()
    : m_isActive(true)
    , m_thread(std::bind(&PostponeLoop::onThreadUpdate, this))
{
}

PostponeLoop::~PostponeLoop()
{
    interrupt();
}

void PostponeLoop::onThreadUpdate()
{
    PSI_EXAMPLE_LOG("Start postpone thread:" << std::this_thread::get_id());

    psi::thread::CrashHandler ch;
    m_crashSub = ch.crashEvent().subscribe([this](const auto &error, const auto &) {
        PSI_EXAMPLE_LOG("Crash in postpone thread: " << std::this_thread::get_id() << ", error: " << error);
    });
    ch.invoke([this]() {
        while (m_isActive) {
            trigger();
        }
    });

    m_isActive = false;
    m_crashSub.reset();

    PSI_EXAMPLE_LOG("Exit postpone thread:" << std::this_thread::get_id());
}

void PostponeLoop::interrupt()
{
    if (m_isActive) {
        m_isActive = false;
        m_condition.notify_all();
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

bool PostponeLoop::isRunning()
{
    return m_isActive;
}

void PostponeLoop::invoke(Func &&fn, const TimePoint &tp)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_queue.empty() || tp < m_nextExecutionTime) {
        m_nextExecutionTime = tp;
        m_condition.notify_one();
    }

    m_queue[tp].emplace_back(fn);
}

void PostponeLoop::trigger()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_queue.empty()) {
        m_condition.wait(lock, [this]() { return !m_queue.empty() || !m_isActive; });
    } else {
        m_condition.wait_until(lock, m_nextExecutionTime, [this]() {
            auto curTime = std::chrono::high_resolution_clock::now();
            return curTime > m_nextExecutionTime || !m_isActive;
        });
    }

    if (m_queue.empty()) {
        return;
    }

    auto curTime = std::chrono::high_resolution_clock::now();
    if (curTime < m_nextExecutionTime) {
        return;
    }

    auto itr = m_queue.begin();
    auto calls = itr->second;
    m_queue.erase(itr);
    if (!m_queue.empty()) {
        itr = m_queue.begin();
        m_nextExecutionTime = itr->first;
    }

    lock.unlock();

    for (auto fn : calls) {
        fn();
    }
}

} // namespace psi::thread