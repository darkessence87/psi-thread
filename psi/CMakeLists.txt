include_directories(
    ${3rdPARTY_DIR}/psi/include
    ${3rdPARTY_DIR}/psi-tools/psi/include
    include
)

link_directories(
    ${3rdPARTY_DIR}/psi/lib/${SUB_DIR_LIBS}
    ${3rdPARTY_DIR}/psi-tools/build/bin/${CMAKE_BUILD_TYPE}
)

set (SOURCES
    src/psi/thread/CrashHandler.cpp
    src/psi/thread/PostponeLoop.cpp
    src/psi/thread/ThreadPool.cpp
    src/psi/thread/ThreadPoolQueued.cpp
    src/psi/thread/Timer.cpp
    src/psi/thread/TimerLoop.cpp
)

add_library(psi-thread STATIC ${SOURCES})
target_link_libraries(psi-thread
    psi-tools
)

set(TEST_SRC
    tests/TimerTests.cpp
)
psi_make_tests("Thread" "${TEST_SRC}" "psi-thread")

set(EXAMPLE_SRC_1.0 examples/1.0_Simple_ThreadPool/EntryPoint.cpp)
psi_make_examples("1.0_Simple_ThreadPool" "${EXAMPLE_SRC_1.0}" "psi-thread")

set(EXAMPLE_SRC_1.1 examples/1.1_Simple_ThreadPoolQueued/EntryPoint.cpp)
psi_make_examples("1.1_Simple_ThreadPoolQueued" "${EXAMPLE_SRC_1.1}" "psi-thread")