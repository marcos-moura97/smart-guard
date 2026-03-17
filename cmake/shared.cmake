include("${CMAKE_SOURCE_DIR}/cmake/warnings.cmake")

macro(new_library LIBRARY_NAME SOURCE_DIR INCLUDE_DIR)
    file(GLOB_RECURSE SOURCES
        CONFIGURE_DEPENDS
            ${SOURCE_DIR}/*.c
            ${SOURCE_DIR}/*.cpp
    )
    add_library(${LIBRARY_NAME} STATIC
        ${SOURCES}
    )
    add_cxx_warnings(${LIBRARY_NAME})
    target_include_directories(${LIBRARY_NAME} PUBLIC ${INCLUDE_DIR})
endmacro()

macro(new_exe EXE_NAME SOURCE_DIR INCLUDE_DIR ENTRY_POINT)
    file(GLOB_RECURSE SOURCES
        CONFIGURE_DEPENDS
            ${SOURCE_DIR}/*.c
            ${SOURCE_DIR}/*.cpp
    )
    add_executable(${EXE_NAME}
        ${SOURCES}
        ${ENTRY_POINT}
    )
    add_cxx_warnings(${EXE_NAME})
    target_include_directories(${EXE_NAME} PUBLIC ${INCLUDE_DIR})
endmacro()

macro(new_tests EXE_NAME SOURCE_DIR INCLUDE_DIR TESTS_DIR ENTRY_POINT)
    file(GLOB_RECURSE SOURCES
        CONFIGURE_DEPENDS
            ${SOURCE_DIR}/*.c
            ${SOURCE_DIR}/*.cpp
    )
    file(GLOB_RECURSE TESTS
        CONFIGURE_DEPENDS
            ${TESTS_DIR}/*.c
            ${TESTS_DIR}/*.cpp
    )
    add_executable(${TEST_NAME}
        ${SOURCES}
        ${TESTS}
        ${ENTRY_POINT}
    )
    add_cxx_warnings(${EXE_NAME})
    target_include_directories(${EXE_NAME} PUBLIC ${INCLUDE_DIR})
endmacro()

macro(target_link_pico_libraries TARGET)
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