# Set maximum error count
# Change this to whatever you want
set(MAX_ERRORS 32)
set(LINT 0)

function(add_cxx_warnings target)
    if(${LINT})
        # Enable extra warnings and errors
        target_compile_options(${target} PRIVATE 
            -fdiagnostics-color=always
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
            #-Wpadded
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
            -Wunused-macros
            -Wunused-parameter
            -Wunused-value
            -Wunused-variable
            -Wvariadic-macros
            -Wno-braced-scalar-init
        )

        # Compiler specific warnings
        target_compile_options(${target} PRIVATE 
            -fmax-errors=${MAX_ERRORS}
            -Wformat-overflow=2
            -Wunused-local-typedefs
        )
    endif()
endfunction()