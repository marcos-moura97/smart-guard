# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20) # previously set to 17
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_CXX_EXTENSIONS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type") # here you type either pico2 (the board with you) or pico2_w (the one with me)
                                                # You can make this cleaner, if you want :)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# Enable warning flags
#include(warnings.cmake)

project(smart_guard C CXX ASM)

add_compile_definitions(
    TF_LITE_STATIC_MEMORY
    TF_LITE_DISABLE_X86_NEON
    TFLITE_DISABLE_RUY
)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

set(PROJECT_SOURCE_DIR src)
file(GLOB_RECURSE SOURCES
  CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/*.c
    ${PROJECT_SOURCE_DIR}/*.cpp
)

add_executable(smart_guard
    main.cpp
    ${SOURCES}
)

if(PICO_BOARD STREQUAL "pico2_w")
    target_compile_definitions(smart_guard PRIVATE PICO_BOARD_PICO2_W=1)
endif()


pico_set_program_name(smart_guard "smart_guard")
pico_set_program_version(smart_guard "0.1")

# Generate PIO header
pico_generate_pio_header(smart_guard ${CMAKE_CURRENT_LIST_DIR}/smart_guard.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(smart_guard 0)
pico_enable_stdio_usb(smart_guard 1)

# Add the standard library to the build
target_link_libraries(smart_guard
    pico_stdlib
    hardware_i2c
    pico_cyw43_arch_none
)
# Add the standard include files to the build
target_include_directories(smart_guard PUBLIC
    include
)

# Add any user requested libraries
target_link_libraries(smart_guard 
    hardware_pio
)

pico_add_extra_outputs(smart_guard)