include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/googletest-src
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/third_party/googletest-build
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set_target_properties(gtest PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:gtest,INTERFACE_INCLUDE_DIRECTORIES>")
set_target_properties(gtest_main PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:gtest_main,INTERFACE_INCLUDE_DIRECTORIES>")

foreach(tgt gtest gtest_main gmock gmock_main)
    if(TARGET ${tgt})
        if(MSVC)
            target_compile_options(${tgt} PRIVATE /w)
        else()
            target_compile_options(${tgt} PRIVATE -w)
        endif()
    endif()
endforeach()

add_executable(glyph_tests
    distance/test_scalar.cpp
    distance/test_dispatch.cpp
    hnsw/test_graph.cpp
    collection/test_collection.cpp
    collection/test_registry.cpp
    wal/test_wal.cpp
    persistence/test_engine.cpp
)

target_link_libraries(glyph_tests PRIVATE
    Glyph::core
    Glyph::compiler_flags
    gtest_main
)

if(WIN32 AND NOT MSVC)
    get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    add_custom_command(TARGET glyph_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${COMPILER_DIR}/libstdc++-6.dll"
            "${COMPILER_DIR}/libgcc_s_seh-1.dll"
            "${COMPILER_DIR}/libwinpthread-1.dll"
            "$<TARGET_FILE_DIR:glyph_tests>"
        COMMENT "Copying runtime DLLs for test execution"
    )
endif()
