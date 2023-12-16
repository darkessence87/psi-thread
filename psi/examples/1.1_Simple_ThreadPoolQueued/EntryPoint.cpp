#include "psi/thread/ThreadPoolQueued.h"

#include <iostream>

int main()
{
    using namespace psi::thread;

    // current thread (main thread) should run until pool threads finished all tasks

    // create thread pool
    ThreadPoolQueued pool(4);

    // it will run asynchronously
    pool.run();

    // we create N tasks of 2 types: one increments value, second decrements value, X times each task
    const size_t X_TIMES = 100'000;
    const size_t N_TASKS = 1'000;
    std::atomic<int> value = 0;
    auto increment = [&value]() {
        for (size_t x = 0; x < X_TIMES; ++x) {
            ++value;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    };
    auto decrement = [&value]() {
        for (size_t x = 0; x < X_TIMES; ++x) {
            --value;
        }
    };

    // give tasks to pool
    for (size_t x = 0; x < N_TASKS; ++x) {
        pool.invoke(increment);
        pool.invoke(decrement);
    }

    // check value every 500 ms
    const auto startTs = std::chrono::high_resolution_clock::now();
    size_t lastWorkload = pool.getWorkload();
    while (pool.getWorkload() && pool.isRunning()) {
        const size_t tasksPerSec = (lastWorkload - pool.getWorkload()) * 2;
        std::cout << "value: " << value << ", workload: " << pool.getWorkload() << ", tasks per seconds: ~"
                  << tasksPerSec << std::endl;
        lastWorkload = pool.getWorkload();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    const auto endTs = std::chrono::high_resolution_clock::now();
    std::cout << "Finished in: ~" << std::chrono::duration_cast<std::chrono::milliseconds>(endTs - startTs).count()
              << " ms" << std::endl;

    // current thread will be locked until pool finished all remaining tasks
    pool.interrupt();

    std::cout << "value: " << value << std::endl;
}