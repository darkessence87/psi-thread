
#include "psi/thread/TimerLoop.h"
#include "psi/thread/CrashHandler.h"

#include <algorithm>

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

TimerLoop::TimerLoop()
    : m_isActive(true)
    , m_thread(std::bind(&TimerLoop::onThreadUpdate, this))
{
}

TimerLoop::~TimerLoop()
{
    interrupt();
}

void TimerLoop::onThreadUpdate()
{
    PSI_EXAMPLE_LOG("Start timer thread:" << std::this_thread::get_id());

    psi::thread::CrashHandler ch;
    m_crashSub = ch.crashEvent().subscribe([this](const auto &error, const auto &) {
        PSI_EXAMPLE_LOG("Crash in timer thread:" << std::this_thread::get_id() << ", error: " << error);
    });
    ch.invoke([this]() {
        while (m_isActive) {
            trigger();
        }
    });

    m_isActive = false;
    m_crashSub.reset();

    PSI_EXAMPLE_LOG("Exit timer thread:" << std::this_thread::get_id());
}

void TimerLoop::interrupt()
{
    if (m_isActive) {
        m_timersPlan.clear();
        m_queue.clear();
        m_isActive = false;

        m_condition.notify_all();
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

bool TimerLoop::isRunning()
{
    return m_isActive;
}

void TimerLoop::addTimer(std::shared_ptr<Timer> timer, int milliseconds)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (!timer) {
        return;
    }

    auto tp = std::chrono::high_resolution_clock::now();
    tp += std::chrono::milliseconds(milliseconds);

    const bool wasEmpty = m_queue.empty();

    m_queue[tp].emplace_back(timer);
    m_timersPlan[timer->m_timerId] = tp;

    if (wasEmpty || tp < m_queue.begin()->first) {
        if (wasEmpty) {
            m_tempExecutionTime = tp.max();
            m_nextExecutionTime = tp;
        } else {
            m_tempExecutionTime = m_queue.begin()->first;
            m_nextExecutionTime = tp.max();
        }

        m_condition.notify_one();
    }
}

void TimerLoop::restartTimer(size_t id)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    auto itr = m_timersPlan.find(id);
    if (itr == m_timersPlan.end()) {
        PSI_EXAMPLE_LOG("Could not find timer: " << id);
        return;
    }

    auto tp = itr->second;
    auto itr2 = m_queue.find(tp);
    if (itr2 == m_queue.end()) {
        PSI_EXAMPLE_LOG("Could not find time point for timer: " << id);
        m_timersPlan.erase(itr);
        return;
    }

    auto &timers = itr2->second;
    auto itr3 = std::find_if(timers.begin(), timers.end(), [id](auto &t) { return id == t->m_timerId; });
    if (itr3 == timers.end()) {
        PSI_EXAMPLE_LOG("Could not find timer: " << id << " in queue");
        return;
    }

    auto newTp = std::chrono::high_resolution_clock::now();
    newTp += std::chrono::milliseconds((*itr3)->m_length);

    const auto newId = (*itr3)->m_timerId;
    m_queue[newTp].emplace_back(*itr3);
    m_queue.erase(itr2);

    m_timersPlan[newId] = newTp;

    if (m_nextExecutionTime == tp) {
        m_tempExecutionTime = m_queue.begin()->first;
        m_nextExecutionTime = tp.max();

        m_condition.notify_one();
    }
}

void TimerLoop::removeTimer(size_t id)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    auto itr = m_timersPlan.find(id);
    if (itr == m_timersPlan.end()) {
        return;
    }

    auto tp = itr->second;
    auto itr2 = m_queue.find(tp);
    if (itr2 == m_queue.end()) {
        m_timersPlan.erase(itr);
        return;
    }

    auto &timers = itr2->second;
    auto itr3 = std::find_if(timers.begin(), timers.end(), [id](auto &t) { return id == t->m_timerId; });
    if (itr3 == timers.end()) {
        PSI_EXAMPLE_LOG("Could not find timer: " << id << " in queue");
        return;
    }

    timers.erase(itr3);
    if (timers.empty()) {
        m_queue.erase(itr2);
    }
    m_timersPlan.erase(itr);

    if (m_queue.empty()) {
        m_tempExecutionTime = tp.min();
        m_nextExecutionTime = tp.max();

        m_condition.notify_one();
    } else if (m_nextExecutionTime == tp) {
        m_tempExecutionTime = m_queue.begin()->first;
        m_nextExecutionTime = tp.max();

        m_condition.notify_one();
    }
}

void TimerLoop::trigger()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_queue.empty()) {
        m_condition.wait(lock, [this]() { return !m_queue.empty() || !m_isActive; });
    } else {
        m_condition.wait_until(lock, m_nextExecutionTime, [this]() {
            return m_tempExecutionTime < m_nextExecutionTime || !m_isActive;
        });
        if (m_tempExecutionTime < m_nextExecutionTime) {
            m_nextExecutionTime = m_tempExecutionTime;
            m_tempExecutionTime = m_tempExecutionTime.max();
        }
    }

    if (m_queue.empty()) {
        return;
    }

    auto curTime = std::chrono::high_resolution_clock::now();
    if (curTime < m_nextExecutionTime) {
        return;
    }

    auto itr = m_queue.begin();
    PSI_EXAMPLE_LOG("[" << itr->first.time_since_epoch().count() << "] timers.size():" << itr->second.size());
    auto timers = itr->second;
    m_queue.erase(itr);
    if (!m_queue.empty()) {
        itr = m_queue.begin();
        m_nextExecutionTime = itr->first;
    }

    lock.unlock();

    for (auto &timer : timers) {
        timer->invoke();
    }
}

} // namespace psi::thread