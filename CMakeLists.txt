# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

unset(PICO_PLATFORM CACHE)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion RISCV_ZCB_RPI_2_2_0_3)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../cmake/shared.cmake")
    set(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
else()
    set(PROJECT_ROOT "${CMAKE_SOURCE_DIR}")
endif()
# ====================================================================================
# Pull in Raspberry Pi Pico SDK (must be before project)
include("${CMAKE_SOURCE_DIR}/pico_sdk_import.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/shared_pico.cmake")
#

# Add subdirectories
add_subdirectory("${CMAKE_SOURCE_DIR}/lib/smart_guard_core")
add_subdirectory("${CMAKE_SOURCE_DIR}/lib/smart_guard_pico")

# Enable warning flags
include("${CMAKE_SOURCE_DIR}/cmake/warnings.cmake")
add_cxx_warnings(smart_guard)
add_cxx_warnings(smart_guard_core)
add_cxx_warnings(smart_guard_pico)

add_compile_definitions(
    TF_LITE_STATIC_MEMORY
    TF_LITE_DISABLE_X86_NEON
    TFLITE_DISABLE_RUY
)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

set(PROJECT_NAME smart_guard)
project(${PROJECT_NAME} C CXX ASM)
new_exe(${PROJECT_NAME} src include main.cpp)
target_link_pico_libraries(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PUBLIC
    smart_guard_core
    smart_guard_pico
)
new_pico_program(${PROJECT_NAME} "smart_guard" "0.1")

# Generate PIO header
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/smart_guard.pio)
