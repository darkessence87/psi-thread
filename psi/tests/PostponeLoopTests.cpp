
#include "psi/test/psi_mock.h"

#include "psi/thread/PostponeLoop.h"
#include "psi/thread/ThreadPool.h"
#include "psi/thread/ThreadPoolQueued.h"

#include <atomic>
#include <chrono>

using namespace psi::thread;
using namespace psi::test;

// ---------------------------------------------------------------------------
// PostponeLoop
// ---------------------------------------------------------------------------

TEST(PostponeLoop_Tests, isRunning_true_initially)
{
    PostponeLoop loop;
    EXPECT_TRUE(loop.isRunning());
}

TEST(PostponeLoop_Tests, interrupt_stops_loop)
{
    PostponeLoop loop;
    EXPECT_TRUE(loop.isRunning());
    loop.interrupt();
    EXPECT_FALSE(loop.isRunning());
}

TEST(PostponeLoop_Tests, invoke_executes_task_after_delay)
{
    PostponeLoop loop;

    auto cb = MockedFn<std::function<void()>>::create();
    EXPECT_CALL(cb, 1);

    auto tp = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(100);
    loop.invoke(cb->fn(), tp);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

TEST(PostponeLoop_Tests, invoke_interrupted_before_delay)
{
    PostponeLoop loop;

    auto cb = MockedFn<std::function<void()>>::create();
    EXPECT_CALL(cb, 0);

    auto tp = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(2000);
    loop.invoke(cb->fn(), tp);

    loop.interrupt();
    EXPECT_FALSE(loop.isRunning());
}

TEST(PostponeLoop_Tests, invoke_multiple_tasks_same_timepoint)
{
    PostponeLoop loop;

    auto cb1 = MockedFn<std::function<void()>>::create();
    auto cb2 = MockedFn<std::function<void()>>::create();
    EXPECT_CALL(cb1, 1);
    EXPECT_CALL(cb2, 1);

    auto tp = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(100);
    loop.invoke(cb1->fn(), tp);
    loop.invoke(cb2->fn(), tp);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

TEST(PostponeLoop_Tests, invoke_multiple_tasks_ordered)
{
    PostponeLoop loop;

    auto cb1 = MockedFn<std::function<void()>>::create();
    auto cb2 = MockedFn<std::function<void()>>::create();
    EXPECT_CALL(cb1, 1);
    EXPECT_CALL(cb2, 1);

    auto now = std::chrono::high_resolution_clock::now();
    loop.invoke(cb1->fn(), now + std::chrono::milliseconds(100));
    loop.invoke(cb2->fn(), now + std::chrono::milliseconds(200));

    std::this_thread::sleep_for(std::chrono::milliseconds(350));
}

// ---------------------------------------------------------------------------
// ThreadPool
// ---------------------------------------------------------------------------

TEST(ThreadPool_Tests, isRunning_false_before_run)
{
    ThreadPool pool(2);
    EXPECT_FALSE(pool.isRunning());
}

TEST(ThreadPool_Tests, isRunning_true_after_run)
{
    ThreadPool pool(2);
    pool.run();
    EXPECT_TRUE(pool.isRunning());
}

TEST(ThreadPool_Tests, interrupt_stops_pool)
{
    ThreadPool pool(2);
    pool.run();
    EXPECT_TRUE(pool.isRunning());
    pool.interrupt();
    EXPECT_FALSE(pool.isRunning());
}

TEST(ThreadPool_Tests, invoke_executes_task)
{
    ThreadPool pool(2);
    pool.run();

    std::atomic<int> count{0};
    pool.invoke([&count]() { ++count; });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pool.interrupt();

    EXPECT_EQ(count.load(), 1);
}

TEST(ThreadPool_Tests, invoke_multiple_tasks_all_executed)
{
    ThreadPool pool(4);
    pool.run();

    std::atomic<int> count{0};
    for (int i = 0; i < 20; ++i) {
        pool.invoke([&count]() { ++count; });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    pool.interrupt();

    EXPECT_EQ(count.load(), 20);
}

TEST(ThreadPool_Tests, interruptImmediately_stops_pool)
{
    ThreadPool pool(2);
    pool.run();
    EXPECT_TRUE(pool.isRunning());
    pool.interruptImmediately();
    EXPECT_FALSE(pool.isRunning());
}

// ---------------------------------------------------------------------------
// ThreadPoolQueued
// ---------------------------------------------------------------------------

TEST(ThreadPoolQueued_Tests, isRunning_false_before_run)
{
    ThreadPoolQueued pool(2);
    EXPECT_FALSE(pool.isRunning());
}

TEST(ThreadPoolQueued_Tests, isRunning_true_after_run)
{
    ThreadPoolQueued pool(2);
    pool.run();
    EXPECT_TRUE(pool.isRunning());
}

TEST(ThreadPoolQueued_Tests, interrupt_stops_pool)
{
    ThreadPoolQueued pool(2);
    pool.run();
    EXPECT_TRUE(pool.isRunning());
    pool.interrupt();
    EXPECT_FALSE(pool.isRunning());
}

TEST(ThreadPoolQueued_Tests, invoke_executes_task)
{
    ThreadPoolQueued pool(2);
    pool.run();

    std::atomic<int> count{0};
    pool.invoke([&count]() { ++count; });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pool.interrupt();

    EXPECT_EQ(count.load(), 1);
}

TEST(ThreadPoolQueued_Tests, invoke_multiple_tasks_all_executed)
{
    ThreadPoolQueued pool(4);
    pool.run();

    std::atomic<int> count{0};
    for (int i = 0; i < 20; ++i) {
        pool.invoke([&count]() { ++count; });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    pool.interrupt();

    EXPECT_EQ(count.load(), 20);
}

TEST(ThreadPoolQueued_Tests, interruptImmediately_stops_pool)
{
    ThreadPoolQueued pool(2);
    pool.run();
    EXPECT_TRUE(pool.isRunning());
    pool.interruptImmediately();
    EXPECT_FALSE(pool.isRunning());
}
