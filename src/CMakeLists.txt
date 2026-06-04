add_library(glyph_core STATIC
    distance/scalar.cpp
    distance/avx2.cpp
    distance/neon.cpp
    distance/dispatch.cpp
)

add_library(Glyph::core ALIAS glyph_core)

target_include_directories(glyph_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

target_link_libraries(glyph_core PRIVATE
    Glyph::compiler_flags
    Glyph::strict_flags
)

if(MSVC)
    set_source_files_properties(distance/avx2.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
else()
    set_source_files_properties(distance/avx2.cpp PROPERTIES COMPILE_FLAGS "-mavx2")
endif()
