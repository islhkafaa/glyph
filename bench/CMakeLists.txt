add_executable(glyph_bench main.cpp)

target_link_libraries(glyph_bench PRIVATE
    Glyph::core
    Glyph::compiler_flags
    nlohmann_json::nlohmann_json
)

if(WIN32 AND NOT MSVC)
    get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    add_custom_command(TARGET glyph_bench POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${COMPILER_DIR}/libstdc++-6.dll"
            "${COMPILER_DIR}/libgcc_s_seh-1.dll"
            "${COMPILER_DIR}/libwinpthread-1.dll"
            "$<TARGET_FILE_DIR:glyph_bench>"
    )
endif()
