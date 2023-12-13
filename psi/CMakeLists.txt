if (NOT EXISTS ${psi_comm_dir})
    set (psi_comm_dir "")
    find_submodule(psi-comm psi_comm_dir)
    message("psi_comm_dir: ${psi_comm_dir}")
endif()

if (NOT EXISTS ${psi_tools_dir})
    set (psi_tools_dir "")
    find_submodule(psi-tools psi_tools_dir)
    message("psi_tools_dir: ${psi_tools_dir}")
endif()

include_directories(
    ${3rdPARTY_DIR}/psi/include
    ${psi_comm_dir}/psi/include
    ${psi_tools_dir}/psi/include
    include
)

link_directories(
    ${3rdPARTY_DIR}/psi/lib/${SUB_DIR_LIBS}
    ${psi_tools_dir}/build/bin/${CMAKE_BUILD_TYPE}
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

if (WIN32)
    set (PLATFORM_LIBS "dbghelp.dll" "ntdll.dll")
endif()

target_link_libraries(psi-thread
    ${PLATFORM_LIBS}
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