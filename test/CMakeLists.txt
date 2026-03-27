cmake_minimum_required(VERSION 3.60)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

project(smart_guard_unit_tests C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

include(CTest)
enable_testing()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../cmake/shared.cmake")
    set(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
else()
    set(PROJECT_ROOT "${CMAKE_SOURCE_DIR}")
endif()

add_subdirectory(${PROJECT_ROOT}/lib/smart_guard_core
                    ${PROJECT_ROOT}/smart_guard_core_build)
        

include(FetchContent)
FetchContent_Declare(
    CppUTest
    GIT_REPOSITORY https://github.com/cpputest/cpputest.git
    GIT_TAG v4.0
)

set(TESTS OFF CACHE BOOL "Disable CppUTest self-tests" FORCE)
FetchContent_MakeAvailable(CppUTest)

add_executable(smart_guard_core_test
    #${CMAKE_SOURCE_DIR}/../../test.cpp
    it_works.cpp
    ${PROJECT_ROOT}/test.cpp
)

target_link_libraries(smart_guard_core_test PRIVATE
    smart_guard_core
    CppUTest
    CppUTestExt
)

target_compile_definitions(smart_guard_core_test PRIVATE
    CPPUTEST_MEM_LEAK_DETECTION_DISABLED=1
    CPPUTEST_HAVE_EXCEPTIONS=1
)

add_test(NAME smart_guard_core_test COMMAND smart_guard_core_test)