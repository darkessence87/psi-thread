#pragma once

#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "psi/comm/Event.h"
#include "psi/comm/Subscription.h"
#include "psi/thread/ILoop.h"

namespace psi::thread {

class ThreadPoolQueued : public ILoop
{
    class SimpleThread final
    {
    public:
        SimpleThread();
        ~SimpleThread();

        void run();
        void invoke(Func &&);
        void trigger();
        void interrupt();
        void interruptImmediately();
        bool isRunning();
        size_t getWorkload() const;
        void join();

        void onThreadUpdate();

        using OnCrashEvent = comm::Event<std::string /*error*/, std::string /*stacktrace*/>;
        OnCrashEvent::Interface &onCrashEvent();

    private:
        SimpleThread(const SimpleThread &) = delete;
        SimpleThread &operator=(const SimpleThread &) = delete;

    private:
        std::mutex m_mutex;
        std::condition_variable m_condition;
        std::queue<Func> m_queue;
        bool m_isActive;
        bool m_interruptImmediately;
        std::thread m_thread;
        OnCrashEvent m_onCrashEvent;

        friend class ThreadPoolQueued;
    };

public:
    ThreadPoolQueued(uint8_t numberOfThreads = 10);
    virtual ~ThreadPoolQueued();

public: // ILoop implementation
    void run() override;
    void invoke(Func &&) override;
    void interrupt() override;
    void interruptImmediately() override;
    bool isRunning() override;
    size_t getWorkload() const override;
    void join() override;

private:
    std::atomic<uint8_t> m_threadIndex = 0;
    std::atomic<uint8_t> m_aliveThreads = 0;
    std::vector<std::shared_ptr<SimpleThread>> m_threads;
    std::map<uint8_t, comm::Subscription> m_onCrashSubs;
    uint8_t m_maxNumberOfThreads;
};

} // namespace psi::thread
