# This library contains useful utilities and hardware abstraction layers for the raspberry pico

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

include("${CMAKE_SOURCE_DIR}/pico_sdk_import.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/shared_pico.cmake")

# Not good but i guess it's needed because variables in cmake don't seem to predictably carry over to subfolder cmakelists... or not, either way it works for now
if(NOT DEFINED PICO_BOARD)
    set(PICO_BOARD pico2_w CACHE INTERNAL "")
endif()

set(PROJECT_NAME smart_guard_pico)
project(${PROJECT_NAME} C CXX ASM)
new_library(${PROJECT_NAME} src include)
target_link_pico_libraries(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PUBLIC
  smart_guard_core
)

# ------------------------------------------------------------ CppUTest ----------------------------------------------------------------

set(ENABLE_TESTS OFF)
if(${ENABLE_TESTS})
    set(TEST_NAME "smart_guard_pico_test")
    set(PICO_CXX_ENABLE_EXCEPTIONS 1)
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
    target_link_pico_libraries(${TEST_NAME})
    target_link_libraries(${TEST_NAME} PUBLIC
        smart_guard_core
    )
    #new_pico_program(${TEST_NAME} ${TEST_NAME} "0.1")

    # Pull in our pico_stdlib which pulls in commonly used features
    target_link_libraries(${TEST_NAME} PRIVATE
        CppUTest::CppUTest
        CppUTest::CppUTestExt
    )

    target_compile_definitions(${TEST_NAME} PRIVATE
        CPPUTEST_HAVE_EXCEPTIONS=1
    )
endif()