include("${CMAKE_SOURCE_DIR}/pico_sdk_import.cmake")
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

macro(target_link_pico_libraries TARGET)
    if(PICO_BOARD STREQUAL "pico2_w")
        target_compile_definitions(${TARGET} PRIVATE 
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
    target_link_libraries(${TARGET} PRIVATE
        pico_stdlib
        hardware_i2c
    )
    if(PICO_BOARD STREQUAL "pico2_w")
        target_link_libraries(${TARGET} PRIVATE
            pico_cyw43_arch_lwip_threadsafe_background
            pico_lwip_mqtt
            pico_mbedtls
            pico_lwip_mbedtls
        )
    endif()
    # Add any user requested libraries
    target_link_libraries(${TARGET} PRIVATE
        hardware_pio
    )
endmacro()

macro(new_pico_program EXE_NAME PICO_PROGRAM_NAME PICO_PROGRAM_VERSION)
    pico_set_program_name(${EXE_NAME} ${PICO_PROGRAM_NAME})
    pico_set_program_version(${EXE_NAME} ${PICO_PROGRAM_VERSION})

    # enable usb output, disable uart output
    pico_enable_stdio_uart(${EXE_NAME} 0)
    pico_enable_stdio_usb(${EXE_NAME} 1)
    target_compile_definitions(${EXE_NAME} PRIVATE
        PICO_DEFAULT_UART_RX_PIN=16
        PICO_DEFAULT_UART_TX_PIN=17
    )

    # create map/bin/hex file etc.
    pico_add_extra_outputs(${EXE_NAME})
endmacro()