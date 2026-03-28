# This library contains components that don't rely on any hardware-specific parts, and can seamlessly be tested on any device, not just the raspberry pico

cmake_minimum_required(VERSION 3.60)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

# Enable warning flags
include("${PROJECT_ROOT}/cmake/shared.cmake")

set(PROJECT_NAME smart_guard_core)
project(${PROJECT_NAME} C CXX ASM)
new_library(${PROJECT_NAME} src include)

