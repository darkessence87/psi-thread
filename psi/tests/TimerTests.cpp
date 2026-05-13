
#include "psi/test/psi_mock.h"

#include "psi/thread/Timer.h"
#include "psi/thread/TimerLoop.h"

using namespace psi::thread;
using namespace psi::test;

struct TimerTests {
    using TimerCallback = std::function<void()>;

    TimerTests()
    {
        m_timerLoop = std::make_shared<TimerLoop>();

        m_timer1 = std::make_shared<Timer>(++m_timerCounter, *m_timerLoop);
        m_timer2 = std::make_shared<Timer>(++m_timerCounter, *m_timerLoop);
        m_timer3 = std::make_shared<Timer>(++m_timerCounter, *m_timerLoop);
        m_timer4 = std::make_shared<Timer>(++m_timerCounter, *m_timerLoop);
        m_timer5 = std::make_shared<Timer>(++m_timerCounter, *m_timerLoop);

        m_timer1Cb = MockedFn<TimerCallback>::create();
        m_timer2Cb = MockedFn<TimerCallback>::create();
        m_timer3Cb = MockedFn<TimerCallback>::create();
        m_timer4Cb = MockedFn<TimerCallback>::create();
        m_timer5Cb = MockedFn<TimerCallback>::create();
    }

    std::shared_ptr<TimerLoop> m_timerLoop;
    std::atomic<size_t> m_timerCounter;

    std::shared_ptr<Timer> m_timer1;
    std::shared_ptr<Timer> m_timer2;
    std::shared_ptr<Timer> m_timer3;
    std::shared_ptr<Timer> m_timer4;
    std::shared_ptr<Timer> m_timer5;

    std::shared_ptr<MockedFn<TimerCallback>> m_timer1Cb;
    std::shared_ptr<MockedFn<TimerCallback>> m_timer2Cb;
    std::shared_ptr<MockedFn<TimerCallback>> m_timer3Cb;
    std::shared_ptr<MockedFn<TimerCallback>> m_timer4Cb;
    std::shared_ptr<MockedFn<TimerCallback>> m_timer5Cb;
};

// Actions: start(1), stop(2), restart(3), finished(4), running(5)
// Possible 13 cases:
// 1111
// 1f23     = start -> finished -> stop -> restart
// 1r23     = start -> running -> stop -> restart
// 1f3f2    = start -> finished -> restart -> finished -> stop
// 1f3r2    = start -> finished -> restart -> running -> stop
// 1r3f2    = start -> running -> restart -> finished -> stop
// 1r3r2    = start -> running -> restart -> running -> stop
// 21f3     = stop -> start -> finished -> restart
// 21r3     = stop -> start -> running -> restart
// 23_1     = stop -> restart -> finished/running -> start
// 3_1f2    = restart -> finished/running -> start -> finished -> stop
// 3_1r2    = restart -> finished/running -> start -> running -> stop
// 3_21     = restart -> finished/running -> stop -> start

TEST(Timer_Tests, SingleTimer_StartSpam)
{
    TimerTests test;
    // InSequence dummy;

    EXPECT_CALL(test.m_timer1Cb, 1);

    // start spam
    for (int i = 0; i < 100; ++i) {
        test.m_timer1->start(100, test.m_timer1Cb->fn());
        EXPECT_TRUE(test.m_timer1->isRunning());
    }
    // finished
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_FALSE(test.m_timer1->isRunning());
}

TEST(Timer_Tests, SingleTimer_StartFinishedStopRestart)
{
    TimerTests test;
    // InSequence dummy;

    // start
    EXPECT_CALL(test.m_timer1Cb, 1);
    test.m_timer1->start(100, test.m_timer1Cb->fn());
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // finished
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_FALSE(test.m_timer1->isRunning());
    // stop
    test.m_timer1->stop();
    EXPECT_FALSE(test.m_timer1->isRunning());
    // restart
    EXPECT_CALL(test.m_timer1Cb, 0);
    test.m_timer1->restart();
    EXPECT_FALSE(test.m_timer1->isRunning());
}

TEST(Timer_Tests, SingleTimer_StartRunningStopRestart)
{
    TimerTests test;
    // InSequence dummy;

    // start
    EXPECT_CALL(test.m_timer1Cb, 0);
    test.m_timer1->start(100, test.m_timer1Cb->fn());
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // stop
    test.m_timer1->stop();
    EXPECT_FALSE(test.m_timer1->isRunning());
    // restart
    EXPECT_CALL(test.m_timer1Cb, 0);
    test.m_timer1->restart();
    EXPECT_FALSE(test.m_timer1->isRunning());
}

TEST(Timer_Tests, SingleTimer_StartFinishedRestartFinishedStop)
{
    TimerTests test;
    // InSequence dummy;

    // start
    EXPECT_CALL(test.m_timer1Cb, 1);
    test.m_timer1->start(100, test.m_timer1Cb->fn());
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // finished
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_FALSE(test.m_timer1->isRunning());

    TestLib::verify_and_clear_expectations();

    // restart
    EXPECT_CALL(test.m_timer1Cb, 1);
    test.m_timer1->restart();
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // finished
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_FALSE(test.m_timer1->isRunning());
    // stop
    test.m_timer1->stop();
    EXPECT_FALSE(test.m_timer1->isRunning());
}

TEST(Timer_Tests, SingleTimer_StartFinishedRestartRunningStop)
{
    TimerTests test;
    // InSequence dummy;

    // start
    EXPECT_CALL(test.m_timer1Cb, 1);
    test.m_timer1->start(100, test.m_timer1Cb->fn());
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // finished
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_FALSE(test.m_timer1->isRunning());
    // restart
    EXPECT_CALL(test.m_timer1Cb, 0);
    test.m_timer1->restart();
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // stop
    test.m_timer1->stop();
    EXPECT_FALSE(test.m_timer1->isRunning());
}

TEST(Timer_Tests, SingleTimer_StartRunningRestartFinishedStop)
{
    TimerTests test;
    // InSequence dummy;

    // start
    EXPECT_CALL(test.m_timer1Cb, 0);
    test.m_timer1->start(100, test.m_timer1Cb->fn());
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());

    TestLib::verify_and_clear_expectations();

    // restart
    EXPECT_CALL(test.m_timer1Cb, 1);
    test.m_timer1->restart();
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // finished
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_FALSE(test.m_timer1->isRunning());
    // stop
    test.m_timer1->stop();
    EXPECT_FALSE(test.m_timer1->isRunning());
}

TEST(Timer_Tests, SingleTimer_StartRunningRestartRunningStop)
{
    TimerTests test;
    // InSequence dummy;

    // start
    EXPECT_CALL(test.m_timer1Cb, 0);
    test.m_timer1->start(100, test.m_timer1Cb->fn());
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // restart
    EXPECT_CALL(test.m_timer1Cb, 0);
    test.m_timer1->restart();
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // stop
    test.m_timer1->stop();
    EXPECT_FALSE(test.m_timer1->isRunning());
}

TEST(Timer_Tests, SingleTimer_StopStartFinishedRestart)
{
    TimerTests test;
    // InSequence dummy;

    // stop
    test.m_timer1->stop();
    EXPECT_FALSE(test.m_timer1->isRunning());
    // start
    EXPECT_CALL(test.m_timer1Cb, 1);
    test.m_timer1->start(100, test.m_timer1Cb->fn());
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // finished
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_FALSE(test.m_timer1->isRunning());

    TestLib::verify_and_clear_expectations();

    // restart
    EXPECT_CALL(test.m_timer1Cb, 1);
    test.m_timer1->restart();
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // finished
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_FALSE(test.m_timer1->isRunning());
}

TEST(Timer_Tests, SingleTimer_StopStartRunningRestart)
{
    TimerTests test;
    // InSequence dummy;

    // stop
    test.m_timer1->stop();
    EXPECT_FALSE(test.m_timer1->isRunning());
    // start
    EXPECT_CALL(test.m_timer1Cb, 0);
    test.m_timer1->start(100, test.m_timer1Cb->fn());
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());

    TestLib::verify_and_clear_expectations();
    
    // restart
    EXPECT_CALL(test.m_timer1Cb, 1);
    test.m_timer1->restart();
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // finished
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_FALSE(test.m_timer1->isRunning());
}

TEST(Timer_Tests, SingleTimer_StopRestartStart)
{
    TimerTests test;
    // InSequence dummy;

    // stop
    test.m_timer1->stop();
    EXPECT_FALSE(test.m_timer1->isRunning());
    // restart
    EXPECT_CALL(test.m_timer1Cb, 0);
    test.m_timer1->restart();
    EXPECT_FALSE(test.m_timer1->isRunning());

    TestLib::verify_and_clear_expectations();

    // start
    EXPECT_CALL(test.m_timer1Cb, 1);
    test.m_timer1->start(100, test.m_timer1Cb->fn());
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // finished
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_FALSE(test.m_timer1->isRunning());
}

TEST(Timer_Tests, SingleTimer_RestartStartFinishedStop)
{
    TimerTests test;
    // InSequence dummy;

    // restart
    EXPECT_CALL(test.m_timer1Cb, 0);
    test.m_timer1->restart();
    EXPECT_FALSE(test.m_timer1->isRunning());

    TestLib::verify_and_clear_expectations();
    
    // start
    EXPECT_CALL(test.m_timer1Cb, 1);
    test.m_timer1->start(100, test.m_timer1Cb->fn());
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // finished
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_FALSE(test.m_timer1->isRunning());
    // stop
    test.m_timer1->stop();
    EXPECT_FALSE(test.m_timer1->isRunning());
}

TEST(Timer_Tests, SingleTimer_RestartStartRunningStop)
{
    TimerTests test;
    // InSequence dummy;

    // restart
    EXPECT_CALL(test.m_timer1Cb, 0);
    test.m_timer1->restart();
    EXPECT_FALSE(test.m_timer1->isRunning());
    // start
    EXPECT_CALL(test.m_timer1Cb, 0);
    test.m_timer1->start(100, test.m_timer1Cb->fn());
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // stop
    test.m_timer1->stop();
    EXPECT_FALSE(test.m_timer1->isRunning());
}

TEST(Timer_Tests, SingleTimer_RestartStopStart)
{
    TimerTests test;
    // InSequence dummy;

    // restart
    EXPECT_CALL(test.m_timer1Cb, 0);
    test.m_timer1->restart();
    EXPECT_FALSE(test.m_timer1->isRunning());

    TestLib::verify_and_clear_expectations();
    
    // stop
    test.m_timer1->stop();
    EXPECT_FALSE(test.m_timer1->isRunning());
    // start
    EXPECT_CALL(test.m_timer1Cb, 1);
    test.m_timer1->start(100, test.m_timer1Cb->fn());
    // running
    EXPECT_TRUE(test.m_timer1->isRunning());
    // finished
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_FALSE(test.m_timer1->isRunning());
}

TEST(Timer_Tests, MultipleTimers_OrderByFastest)
{
    TimerTests test;
    // InSequence dummy;

    EXPECT_CALL(test.m_timer1Cb, 1);
    EXPECT_CALL(test.m_timer2Cb, 1);
    EXPECT_CALL(test.m_timer3Cb, 1);
    EXPECT_CALL(test.m_timer4Cb, 1);
    EXPECT_CALL(test.m_timer5Cb, 1);

    // start in order by fastest timer
    test.m_timer1->start(100, test.m_timer1Cb->fn());
    test.m_timer2->start(110, test.m_timer2Cb->fn());
    test.m_timer3->start(120, test.m_timer3Cb->fn());
    test.m_timer4->start(130, test.m_timer4Cb->fn());
    test.m_timer5->start(140, test.m_timer5Cb->fn());

    EXPECT_TRUE(test.m_timer1->isRunning());
    EXPECT_TRUE(test.m_timer2->isRunning());
    EXPECT_TRUE(test.m_timer3->isRunning());
    EXPECT_TRUE(test.m_timer4->isRunning());
    EXPECT_TRUE(test.m_timer5->isRunning());

    // finished all
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(test.m_timer1->isRunning());
    EXPECT_FALSE(test.m_timer2->isRunning());
    EXPECT_FALSE(test.m_timer3->isRunning());
    EXPECT_FALSE(test.m_timer4->isRunning());
    EXPECT_FALSE(test.m_timer5->isRunning());
}

TEST(Timer_Tests, MultipleTimers_OrderBySlowest)
{
    TimerTests test;
    // InSequence dummy;

    EXPECT_CALL(test.m_timer5Cb, 1);
    EXPECT_CALL(test.m_timer4Cb, 1);
    EXPECT_CALL(test.m_timer3Cb, 1);
    EXPECT_CALL(test.m_timer2Cb, 1);
    EXPECT_CALL(test.m_timer1Cb, 1);

    // start in order by fastest timer
    test.m_timer1->start(140, test.m_timer1Cb->fn());
    test.m_timer2->start(130, test.m_timer2Cb->fn());
    test.m_timer3->start(120, test.m_timer3Cb->fn());
    test.m_timer4->start(110, test.m_timer4Cb->fn());
    test.m_timer5->start(100, test.m_timer5Cb->fn());

    EXPECT_TRUE(test.m_timer1->isRunning());
    EXPECT_TRUE(test.m_timer2->isRunning());
    EXPECT_TRUE(test.m_timer3->isRunning());
    EXPECT_TRUE(test.m_timer4->isRunning());
    EXPECT_TRUE(test.m_timer5->isRunning());

    // finished all
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(test.m_timer1->isRunning());
    EXPECT_FALSE(test.m_timer2->isRunning());
    EXPECT_FALSE(test.m_timer3->isRunning());
    EXPECT_FALSE(test.m_timer4->isRunning());
    EXPECT_FALSE(test.m_timer5->isRunning());
}

TEST(Timer_Tests, MultipleTimers_OrderByFastestRestartToNonEqualTime)
{
    TimerTests test;
    // InSequence dummy;

    EXPECT_CALL(test.m_timer2Cb, 1);
    EXPECT_CALL(test.m_timer1Cb, 1);
    EXPECT_CALL(test.m_timer4Cb, 1);
    EXPECT_CALL(test.m_timer3Cb, 1);
    EXPECT_CALL(test.m_timer5Cb, 1);

    // start in order by fastest timer
    test.m_timer1->start(1000, test.m_timer1Cb->fn());
    test.m_timer2->start(1500, test.m_timer2Cb->fn());
    test.m_timer3->start(2000, test.m_timer3Cb->fn());
    test.m_timer4->start(2500, test.m_timer4Cb->fn());
    test.m_timer5->start(3000, test.m_timer5Cb->fn());

    EXPECT_TRUE(test.m_timer1->isRunning());
    EXPECT_TRUE(test.m_timer2->isRunning());
    EXPECT_TRUE(test.m_timer3->isRunning());
    EXPECT_TRUE(test.m_timer4->isRunning());
    EXPECT_TRUE(test.m_timer5->isRunning());

    std::this_thread::sleep_for(std::chrono::milliseconds(750));
    test.m_timer1->restart(); // 1750
                         // 1500
    test.m_timer3->restart(); // 2750
                         // 2500
    test.m_timer5->restart(); // 3750

    // finished all
    std::this_thread::sleep_for(std::chrono::milliseconds(3500));
    EXPECT_FALSE(test.m_timer1->isRunning());
    EXPECT_FALSE(test.m_timer2->isRunning());
    EXPECT_FALSE(test.m_timer3->isRunning());
    EXPECT_FALSE(test.m_timer4->isRunning());
    EXPECT_FALSE(test.m_timer5->isRunning());
}

TEST(Timer_Tests, MultipleTimers_OrderByFastestRestartToEqualTime)
{
    TimerTests test;
    // InSequence dummy;

    EXPECT_CALL(test.m_timer2Cb, 1);
    EXPECT_CALL(test.m_timer1Cb, 1);
    EXPECT_CALL(test.m_timer4Cb, 1);
    EXPECT_CALL(test.m_timer3Cb, 1);
    EXPECT_CALL(test.m_timer5Cb, 1);

    // start in order by fastest timer
    test.m_timer1->start(1000, test.m_timer1Cb->fn());
    test.m_timer2->start(1500, test.m_timer2Cb->fn());
    test.m_timer3->start(2000, test.m_timer3Cb->fn());
    test.m_timer4->start(2500, test.m_timer4Cb->fn());
    test.m_timer5->start(3000, test.m_timer5Cb->fn());

    EXPECT_TRUE(test.m_timer1->isRunning());
    EXPECT_TRUE(test.m_timer2->isRunning());
    EXPECT_TRUE(test.m_timer3->isRunning());
    EXPECT_TRUE(test.m_timer4->isRunning());
    EXPECT_TRUE(test.m_timer5->isRunning());

    // sleep 50 ms -> restart
    std::this_thread::sleep_for(std::chrono::milliseconds(510));
    test.m_timer1->restart(); // 1510
                         // 1500
    test.m_timer3->restart(); // 2510
                         // 2500
    test.m_timer5->restart(); // 3510

    // finished all
    std::this_thread::sleep_for(std::chrono::milliseconds(3500));
    EXPECT_FALSE(test.m_timer1->isRunning());
    EXPECT_FALSE(test.m_timer2->isRunning());
    EXPECT_FALSE(test.m_timer3->isRunning());
    EXPECT_FALSE(test.m_timer4->isRunning());
    EXPECT_FALSE(test.m_timer5->isRunning());
}

TEST(Timer_Tests, MultipleTimers_OrderBySlowestRestartToNonEqualTime)
{
    TimerTests test;
    // InSequence dummy;

    EXPECT_CALL(test.m_timer4Cb, 1);
    EXPECT_CALL(test.m_timer5Cb, 1);
    EXPECT_CALL(test.m_timer2Cb, 1);
    EXPECT_CALL(test.m_timer3Cb, 1);
    EXPECT_CALL(test.m_timer1Cb, 1);

    // start in order by slowest timer
    test.m_timer1->start(3000, test.m_timer1Cb->fn());
    test.m_timer2->start(2500, test.m_timer2Cb->fn());
    test.m_timer3->start(2000, test.m_timer3Cb->fn());
    test.m_timer4->start(1500, test.m_timer4Cb->fn());
    test.m_timer5->start(1000, test.m_timer5Cb->fn());

    EXPECT_TRUE(test.m_timer1->isRunning());
    EXPECT_TRUE(test.m_timer2->isRunning());
    EXPECT_TRUE(test.m_timer3->isRunning());
    EXPECT_TRUE(test.m_timer4->isRunning());
    EXPECT_TRUE(test.m_timer5->isRunning());

    std::this_thread::sleep_for(std::chrono::milliseconds(750));
    test.m_timer1->restart(); // 3750
                         // 2500
    test.m_timer3->restart(); // 2750
                         // 1500
    test.m_timer5->restart(); // 1750

    // finished all
    std::this_thread::sleep_for(std::chrono::milliseconds(3500));
    EXPECT_FALSE(test.m_timer1->isRunning());
    EXPECT_FALSE(test.m_timer2->isRunning());
    EXPECT_FALSE(test.m_timer3->isRunning());
    EXPECT_FALSE(test.m_timer4->isRunning());
    EXPECT_FALSE(test.m_timer5->isRunning());
}

TEST(Timer_Tests, TimerLoop_IsRunning)
{
    auto loop = std::make_shared<TimerLoop>();
    EXPECT_TRUE(loop->isRunning());
    loop->interrupt();
    EXPECT_FALSE(loop->isRunning());
}

TEST(Timer_Tests, AddTimer_Nullptr)
{
    TimerTests test;
    test.m_timerLoop->addTimer(nullptr, 100);
}

TEST(Timer_Tests, RestartTimer_UnknownId)
{
    TimerTests test;
    test.m_timerLoop->restartTimer(99999u);
}

TEST(Timer_Tests, RemoveTimer_UnknownId)
{
    TimerTests test;
    test.m_timerLoop->removeTimer(99999u);
}

TEST(Timer_Tests, SingleTimer_StartNegativeTime_ImmediateCallback)
{
    TimerTests test;
    EXPECT_CALL(test.m_timer1Cb, 1);
    test.m_timer1->start(-1, test.m_timer1Cb->fn());
}

TEST(Timer_Tests, MultipleTimers_RemoveWhileRunning)
{
    TimerTests test;

    EXPECT_CALL(test.m_timer1Cb, 0);
    EXPECT_CALL(test.m_timer2Cb, 1);

    test.m_timer1->start(200, test.m_timer1Cb->fn());
    test.m_timer2->start(200, test.m_timer2Cb->fn());

    EXPECT_TRUE(test.m_timer1->isRunning());
    EXPECT_TRUE(test.m_timer2->isRunning());

    test.m_timer1->stop();
    EXPECT_FALSE(test.m_timer1->isRunning());

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    EXPECT_FALSE(test.m_timer2->isRunning());
}
