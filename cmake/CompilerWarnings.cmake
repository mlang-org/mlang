# Centralised warning configuration applied via the mlang::warnings interface.
add_library(mlang_warnings INTERFACE)
add_library(mlang::warnings ALIAS mlang_warnings)

if(MSVC)
    target_compile_options(mlang_warnings INTERFACE /W4 /permissive- /EHsc)
    if(MLANG_WARNINGS_AS_ERRORS)
        target_compile_options(mlang_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(mlang_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wnull-dereference
        -Wdouble-promotion)
    if(MLANG_WARNINGS_AS_ERRORS)
        target_compile_options(mlang_warnings INTERFACE -Werror)
    endif()
endif()
