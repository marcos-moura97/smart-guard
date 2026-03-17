# This library contains components that don't rely on any hardware-specific parts, and can seamlessly be tested on any device, not just the raspberry pico

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

# Enable warning flags
include("${CMAKE_SOURCE_DIR}/cmake/warnings.cmake")

set(PROJECT smart_guard_core)
project(${PROJECT} C CXX ASM)
set(PROJECT_SOURCE_DIR src)
set(PROJECT_INCLUDE_DIR include)

file(GLOB_RECURSE SOURCES
  CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/*.c
    ${PROJECT_SOURCE_DIR}/*.cpp
)
add_library(${PROJECT} STATIC
  ${SOURCES}
)
#add_cxx_warnings(${PROJECT})
target_include_directories(${PROJECT} PUBLIC ${PROJECT_INCLUDE_DIR})