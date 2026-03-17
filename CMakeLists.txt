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

if(PICO_BOARD STREQUAL "pico2_w")
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
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
target_link_pico_libraries(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PUBLIC
    smart_guard_core
    smart_guard_pico
)
new_pico_program(${PROJECT_NAME} "smart_guard" "0.1")

# Generate PIO header
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/smart_guard.pio)

# ------------------------------------------------------------ CppUTest ----------------------------------------------------------------

set(ENABLE_TESTS ON)
if(${ENABLE_TESTS})
    set(TEST_NAME "smart_guard_test")
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

    if(PICO_BOARD STREQUAL "pico2_w")
        target_compile_definitions(${TEST_NAME} PRIVATE 
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
    target_link_pico_libraries(${TEST_NAME})
    target_link_libraries(${TEST_NAME} PUBLIC
        smart_guard_core
        smart_guard_pico
    )
    new_pico_program(${TEST_NAME} "smart_guard_test" "0.1")

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