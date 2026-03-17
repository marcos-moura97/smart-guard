if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/config.cmake")
    include("${CMAKE_SOURCE_DIR}/cmake/config.cmake")
else()
    message(WARNING 
        "Missing config.cmake\n"
        "Some variables can be missing. Create cmake/config.cmake from cmake/config.cmake.example"
    )
endif()
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