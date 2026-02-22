#include "psi/thread/CrashHandler.h"

#include <iostream>
#include <map>
#include <thread>

int main()
{
    using namespace psi::thread;

    const auto threadId = std::this_thread::get_id();
    std::cout << "Current thread: " << threadId << std::endl;
    
    std::map<std::thread::id, psi::comm::Subscription> m_onCrashSubs;

    auto runThread = []() {
        // access violation
        // int* p = nullptr;
        // *p = 42;

        // read AV
        volatile int x = *(int*)0x1;

        // execute AV
        // using Fn = void (*)();
        // Fn f = (Fn)0x1;
        // f();

        // illegal instruction
        // __ud2();

        // divide by zero (integer)
        // int x = 1 / 0;

        // divide by zero (floating)
        // volatile double xx = 1.0 / 0.0;

        // stack overflow
        // std::function<void()> ff;
        // ff = [&ff]() {
        //     ff();
        // };
        // ff();

        // throw std::runtime_error("test");
        // while (true) new char[1024 * 1024];
        // throw 42;

        // misaligned access
        // alignas(1) char buf[sizeof(int)];
        // int *p = reinterpret_cast<int *>(buf);
        // *p = 1;
    };

    CrashHandler ch;
    {
        m_onCrashSubs[threadId] = ch.crashEvent().subscribe([](const auto &error, const auto &stacktrace) {
            std::cout << "Crash in pool thread: " << std::this_thread::get_id() << ", error: [" << error << "]" << std::endl;
            std::cout << stacktrace << std::endl;
        });
    }

    ch.invoke(runThread);
    {
        m_onCrashSubs.erase(m_onCrashSubs.find(threadId));
    }

    std::cout << "Exit current thread: " << threadId << std::endl;
}
