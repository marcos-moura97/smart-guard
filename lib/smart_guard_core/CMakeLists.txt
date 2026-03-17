# This library contains components that don't rely on any hardware-specific parts, and can seamlessly be tested on any device, not just the raspberry pico

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

# Enable warning flags
include("${CMAKE_SOURCE_DIR}/cmake/shared.cmake")

set(PROJECT_NAME smart_guard_core)
project(${PROJECT_NAME} C CXX ASM)
new_library(${PROJECT_NAME} src include)

# ------------------------------------------------------------ CppUTest ----------------------------------------------------------------

set(ENABLE_TESTS ON)
if(${ENABLE_TESTS})
    set(TEST_NAME "smart_guard_core_test")
    set(CPPUTEST_MEM_LEAK_DETECTION_DISABLED 1) # causes compile errors in standard library, so disabling this for now.

    # CppUTest
    include(FetchContent)
    FetchContent_Declare(
        CppUTest
        GIT_REPOSITORY https://github.com/cpputest/cpputest.git
        GIT_TAG        master # or use release tag, eg. v4.0
    )
    # Set this to ON if you want to have the CppUTests in your project as well.
    set(TESTS OFF CACHE BOOL "Switch off CppUTest Test build")
    FetchContent_MakeAvailable(CppUTest)

    new_tests(${TEST_NAME} src include test test.cpp)

    # Pull in our pico_stdlib which pulls in commonly used features
    target_link_libraries(${TEST_NAME} PRIVATE
        CppUTest::CppUTest
        CppUTest::CppUTestExt
    )

    target_compile_definitions(${TEST_NAME} PRIVATE
        PICO_CXX_DISABLE_ALLOCATION_OVERRIDES
        CPPUTEST_HAVE_EXCEPTIONS=1
    )
endif()