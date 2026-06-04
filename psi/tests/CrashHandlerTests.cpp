
#include "psi/test/psi_mock.h"

#include "psi/thread/CrashHandler.h"
#include "psi/comm/Subscription.h"

#include <string>

using namespace psi::thread;
using namespace psi::test;

namespace {

struct Fixture {
    CrashHandler handler;
    bool fired = false;
    std::string error;
    std::string stacktrace;
    psi::comm::Subscription subscription;

    Fixture()
    {
        subscription = handler.crashEvent().subscribe([this](const std::string &err, const std::string &st) {
            fired = true;
            error = err;
            stacktrace = st;
        });
    }
};

} // namespace

TEST(CrashHandler_Tests, no_event_when_no_exception)
{
    Fixture f;
    f.handler.invoke([] {});
    EXPECT_FALSE(f.fired);
}

TEST(CrashHandler_Tests, event_fires_on_std_runtime_error)
{
    Fixture f;
    f.handler.invoke([] { throw std::runtime_error("runtime_boom"); });
    EXPECT_TRUE(f.fired);
    EXPECT_TRUE(f.error.find("runtime_boom") != std::string::npos);
}

TEST(CrashHandler_Tests, event_fires_on_std_logic_error)
{
    Fixture f;
    f.handler.invoke([] { throw std::logic_error("logic_boom"); });
    EXPECT_TRUE(f.fired);
    EXPECT_TRUE(f.error.find("logic_boom") != std::string::npos);
}

TEST(CrashHandler_Tests, event_fires_on_catchall)
{
    Fixture f;
    f.handler.invoke([] { throw 42; });
    EXPECT_TRUE(f.fired);
}

#ifdef _WIN32
TEST(CrashHandler_Tests, event_fires_on_access_violation)
{
    Fixture f;
    f.handler.invoke([] {
        volatile int *p = nullptr;
        *p = 1;
    });
    EXPECT_TRUE(f.fired);
    EXPECT_FALSE(f.error.empty());
}

TEST(CrashHandler_Tests, event_fires_on_divide_by_zero)
{
    Fixture f;
    f.handler.invoke([] {
        volatile int x = 0;
        volatile int y = 1 / x;
        (void)y;
    });
    EXPECT_TRUE(f.fired);
    EXPECT_FALSE(f.error.empty());
}

TEST(CrashHandler_Tests, stacktrace_not_empty_on_access_violation)
{
    Fixture f;
    f.handler.invoke([] {
        volatile int *p = nullptr;
        *p = 1;
    });
    EXPECT_TRUE(f.fired);
    EXPECT_FALSE(f.stacktrace.empty());
}
#endif
