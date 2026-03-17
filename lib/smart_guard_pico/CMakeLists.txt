# This library contains useful utilities and hardware abstraction layers for the raspberry pico

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

include("${CMAKE_SOURCE_DIR}/cmake/warnings.cmake")

set(PROJECT_NAME smart_guard_pico)
project(${PROJECT_NAME} C CXX ASM)
new_library(${PROJECT_NAME} src include)

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
)