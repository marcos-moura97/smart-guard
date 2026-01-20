# Enable extra warnings and errors
add_compile_options(-fdiagnostics-color=always)
add_compile_options(
    -Wall
    -Wextra
    -pedantic
    -Wpedantic
    -Wsign-conversion
    -Wconversion
    -Werror=switch
    -Wdouble-promotion
    -Wnull-dereference
    -Winfinite-recursion
    -Wmisleading-indentation
    -Werror=return-local-addr
    -Wno-c++23-extensions
    -Werror=shadow
    -Wpointer-arith
    -Wcast-qual
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wformat=2
    -Wundef
    -Wuninitialized
    -Winit-self
    #-Wfloat-equal
    -Wwrite-strings
    -Wmissing-field-initializers
    -Wredundant-decls
    -Waggregate-return
    -Wstrict-aliasing=2
    -Warray-bounds
    -Wcast-align
    -Wswitch-default
    -Wswitch-enum
    -Wstack-protector
    -Wold-style-definition
    -Wbad-function-cast
    -Wc++-compat
    -Wdeclaration-after-statement
    -Wmissing-declarations
    -Wmissing-include-dirs
    -Wnested-externs
    -Wpacked
    -Wpadded
    -Wredundant-decls
    -Wsign-compare
    -Wstrict-overflow=5
    -Wvla
    -Wdisabled-optimization
    -Winline
    -Winvalid-pch
    -Wmissing-format-attribute
    -Woverlength-strings
    -Wpragmas
    -Wunused-but-set-parameter
    -Wunused-but-set-variable
    -Wunused-const-variable
    -Wunused-function
    -Wunused-label
    -Wunused-local-typedef
    -Wunused-macros
    -Wunused-parameter
    -Wunused-value
    -Wunused-variable
    -Wvariadic-macros
    -Wno-braced-scalar-init
)

# Set maximum error count
# Change this to whatever you want
set(MAX_ERRORS 32)

# Compiler specific warnings
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fmax-errors=${MAX_ERRORS})
  add_compile_options(-Wformat-overflow=2)
  include(gcc_modules.cmake)
else()
  add_compile_options(-Wstrict-overflow)
endif()