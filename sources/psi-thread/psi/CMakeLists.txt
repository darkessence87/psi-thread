set( CRASH_DUMP_DIR ${BUILD_DIR}/bin/${CMAKE_BUILD_TYPE}/crashdumps)
configure_file(${CMAKE_SOURCE_DIR}/app_config.h.in ${SRC_DIR}/psi/include/psi/app_config.h)
file(MAKE_DIRECTORY ${CRASH_DUMP_DIR})

include_directories(
    include
)

set (SOURCES
    src/psi/thread/CrashHandler.cpp
    src/psi/thread/PostponeLoop.cpp
    src/psi/thread/ThreadPool.cpp
    src/psi/thread/ThreadPoolQueued.cpp
    src/psi/thread/Timer.cpp
    src/psi/thread/TimerLoop.cpp
    src/psi/Tools.cpp
)

add_library(psi-thread STATIC ${SOURCES})

set(TEST_SRC
    tests/TimerTests.cpp
)
psi_make_tests("Thread" "${TEST_SRC}" "psi-thread")

set(EXAMPLE_SRC_1.0 examples/1.0_Simple_ThreadPool/EntryPoint.cpp)
psi_make_examples("1.0_Simple_ThreadPool" "${EXAMPLE_SRC_1.0}" "psi-thread")

set(EXAMPLE_SRC_1.1 examples/1.1_Simple_ThreadPoolQueued/EntryPoint.cpp)
psi_make_examples("1.1_Simple_ThreadPoolQueued" "${EXAMPLE_SRC_1.1}" "psi-thread")