# This library contains useful utilities and hardware abstraction layers for the raspberry pico

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

include("${PROJECT_ROOT}/pico_sdk_import.cmake")
include("${PROJECT_ROOT}/cmake/shared_pico.cmake")

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
