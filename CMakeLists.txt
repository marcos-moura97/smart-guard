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
# ====================================================================================
# Pull in Raspberry Pi Pico SDK (must be before project)
include("${CMAKE_SOURCE_DIR}/pico_sdk_import.cmake")
#
if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/config.cmake")
    include("${CMAKE_SOURCE_DIR}/cmake/config.cmake")
else()
    message(WARNING 
        "Missing config.cmake\n"
        "Some variables can be missing. Create cmake/config.cmake from cmake/config.cmake.example"
    )
endif()
if(NOT DEFINED PICO_BOARD)
    set(PICO_BOARD pico2_w CACHE INTERNAL "")
endif()

# Add subdirectories
add_subdirectory("${CMAKE_SOURCE_DIR}/lib/smart_guard_core")
add_subdirectory("${CMAKE_SOURCE_DIR}/lib/smart_guard_pico")

# Enable warning flags
include("${CMAKE_SOURCE_DIR}/cmake/warnings.cmake")
add_cxx_warnings(smart_guard)
add_cxx_warnings(smart_guard_core)
add_cxx_warnings(smart_guard_pico)

set(PROJECT smart_guard)
project(${PROJECT} C CXX ASM)

add_compile_definitions(
    TF_LITE_STATIC_MEMORY
    TF_LITE_DISABLE_X86_NEON
    TFLITE_DISABLE_RUY
)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

set(PROJECT_SOURCE_DIR src)
set(PROJECT_INCLUDE_DIR include)

file(GLOB_RECURSE SOURCES
  CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/*.c
    ${PROJECT_SOURCE_DIR}/*.cpp
)

add_executable(${PROJECT}
    ${SOURCES}
    main.cpp
)
add_cxx_warnings(${PROJECT})
target_include_directories(${PROJECT} PUBLIC ${PROJECT_INCLUDE_DIR})

if(PICO_BOARD STREQUAL "pico2_w")
    target_compile_definitions(${PROJECT} PRIVATE 
        PICO_BOARD_PICO2_W=1
        WIFI_SSID="${WIFI_SSID}"
        WIFI_PASSWORD="${WIFI_PASSWORD}"
        MQTT_SERVER="${MQTT_SERVER}"
        MQTT_PORT_=${MQTT_PORT_}
        MQTT_TLS_PORT=${MQTT_TLS_PORT}
        MQTT_USERNAME="${MQTT_USERNAME}"
        MQTT_PASSWORD="${MQTT_PASSWORD}"
    )
endif()

# Add the standard library to the build
target_link_libraries(${PROJECT}
    pico_stdlib
    hardware_i2c
)
if(PICO_BOARD STREQUAL "pico2_w")
    target_link_libraries(${PROJECT}
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_mqtt
        pico_mbedtls
        pico_lwip_mbedtls
    )
endif()
# Add any user requested libraries
target_link_libraries(${PROJECT}
    hardware_pio
    smart_guard_core
    smart_guard_pico
)

pico_set_program_name(${PROJECT} "smart_guard")
pico_set_program_version(${PROJECT} "0.1")

# Generate PIO header
pico_generate_pio_header(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/smart_guard.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(${PROJECT} 0)
pico_enable_stdio_usb(${PROJECT} 1)

pico_add_extra_outputs(${PROJECT})