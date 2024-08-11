
#include "psi/thread/Timer.h"
#include "psi/thread/TimerLoop.h"

namespace psi::thread {

Timer::Timer(size_t id, TimerLoop &loop)
    : m_loop(loop)
    , m_timerId(id)
    , m_isActive(false)
    , m_function(nullptr)
    , m_length(0)
    , m_isPeriodic(false)
{
}

Timer::~Timer()
{
    stop();
}

bool Timer::isRunning() const
{
    return m_isActive;
}

void Timer::start(int timeLen, const Func &func)
{
    if (timeLen < 0) {
        func();
        return;
    }

    if (m_isActive) {
        restart();
        return;
    }

    //LOG_INFO("Start timer:" << m_timerId);

    m_function = func;
    m_length = timeLen;

    m_loop.addTimer(shared_from_this(), timeLen);

    m_isActive = true;
}

void Timer::startPeriodic(int timeLen, const Func &func)
{
    m_isPeriodic = true;
    start(timeLen, func);
}

void Timer::restart()
{
    if (!m_function) {
        return;
    }

    //LOG_INFO("Restart timer:" << m_timerId);

    if (!m_isActive) {
        start(m_length, std::move(m_function));
        return;
    }

    m_loop.restartTimer(m_timerId);
}

void Timer::stop()
{
    //LOG_TRACE("Stop timer:" << m_timerId);
    m_isActive = false;
    m_loop.removeTimer(m_timerId);

    m_function = nullptr;
    m_length = 0;
}

void Timer::invoke()
{
    if (m_isActive && m_function) {
        //LOG_TRACE("Call timer:" << m_timerId);
        if (m_isPeriodic) {
            m_loop.addTimer(shared_from_this(), m_length);
        } else {
            m_isActive = false;
        }

        m_function();
    }
}

} // namespace psi::thread