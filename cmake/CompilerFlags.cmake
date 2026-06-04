add_library(compiler_flags INTERFACE)
add_library(Glyph::compiler_flags ALIAS compiler_flags)

add_library(strict_flags INTERFACE)
add_library(Glyph::strict_flags ALIAS strict_flags)

if(MSVC)
    target_compile_options(compiler_flags INTERFACE
        /W4
        /permissive-
        $<$<CONFIG:Debug>:/Od>
        $<$<CONFIG:Debug>:/Ob0>
        $<$<CONFIG:Debug>:/Zi>
        $<$<CONFIG:Debug>:/fsanitize=address>
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/Ob2>
        $<$<CONFIG:Release>:/DNDEBUG>
        $<$<CONFIG:RelWithDebInfo>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/Ob2>
        $<$<CONFIG:RelWithDebInfo>:/Zi>
        $<$<CONFIG:RelWithDebInfo>:/DNDEBUG>
    )
    target_compile_options(strict_flags INTERFACE
        /WX
    )
    target_link_options(compiler_flags INTERFACE
        $<$<CONFIG:Debug>:/incremental:no>
    )
else()
    target_compile_options(compiler_flags INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-DNDEBUG>
        $<$<CONFIG:RelWithDebInfo>:-O3>
        $<$<CONFIG:RelWithDebInfo>:-g>
        $<$<CONFIG:RelWithDebInfo>:-DNDEBUG>
    )
    target_compile_options(strict_flags INTERFACE
        -Werror
    )
    if(NOT WIN32)
        target_compile_options(compiler_flags INTERFACE
            $<$<CONFIG:Debug>:-fsanitize=address>
            $<$<CONFIG:Debug>:-fsanitize=undefined>
        )
        target_link_options(compiler_flags INTERFACE
            $<$<CONFIG:Debug>:-fsanitize=address>
            $<$<CONFIG:Debug>:-fsanitize=undefined>
        )
    endif()
endif()
