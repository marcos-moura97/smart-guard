cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

include("${CMAKE_SOURCE_DIR}/cmake/warnings.cmake")

set(PROJECT smart_guard_pico)
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
)