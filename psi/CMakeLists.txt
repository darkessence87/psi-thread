set (SOURCES
    src/psi/thread/crash_handler/CrashHandler.cpp
    src/psi/thread/crash_handler/stacktrace.cpp
    src/psi/thread/PostponeLoop.cpp
    src/psi/thread/ThreadPool.cpp
    src/psi/thread/ThreadPoolQueued.cpp
    src/psi/thread/Timer.cpp
    src/psi/thread/TimerLoop.cpp
)

if(WIN32)
    list(APPEND SOURCES
        src/psi/thread/crash_handler/dbg_helper.cpp
        src/psi/thread/crash_handler/mini_dump.cpp
        src/psi/thread/crash_handler/seh_handler.cpp
        src/psi/thread/crash_handler/ubsan_handler.cpp
    )
endif()

set(target_lib "psi-thread")

add_library(${target_lib} STATIC ${SOURCES})
add_library(psi::thread ALIAS ${target_lib})

target_include_directories(${target_lib}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (WIN32)
    target_link_libraries(${target_lib} dbghelp.dll ntdll.dll)
endif()

if (ENABLE_LOGGER)
    psi_link(${target_lib} psi-logger)
endif()

psi_config_target(${target_lib})

set(TEST_SRC
    tests/TimerTests.cpp
)
psi_make_tests("Thread" "${TEST_SRC}" "psi-thread")

set(EXAMPLE_SRC_1.0 examples/1.0_Simple_ThreadPool/EntryPoint.cpp)
psi_make_examples("1.0_Simple_ThreadPool" "${EXAMPLE_SRC_1.0}" "psi-thread")

set(EXAMPLE_SRC_1.1 examples/1.1_Simple_ThreadPoolQueued/EntryPoint.cpp)
psi_make_examples("1.1_Simple_ThreadPoolQueued" "${EXAMPLE_SRC_1.1}" "psi-thread")

set(EXAMPLE_SRC_1.2 examples/1.2_CrashHandler/EntryPoint.cpp)
psi_make_examples("1.2_CrashHandler" "${EXAMPLE_SRC_1.2}" "psi-thread")