add_library(glyph_core STATIC
    distance/scalar.cpp
    distance/avx2.cpp
    distance/neon.cpp
    distance/dispatch.cpp
    hnsw/graph.cpp
    quantization/sq8.cpp
    quantization/pq.cpp
    metadata/filter.cpp
    collection/collection.cpp
    collection/registry.cpp
    wal/log.cpp
    persistence/engine.cpp
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

set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../proto/glyph.proto")
set(GEN_PROTO_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GEN_PROTO_DIR}")

set(GEN_SOURCES
    "${GEN_PROTO_DIR}/glyph.pb.cc"
    "${GEN_PROTO_DIR}/glyph.pb.h"
    "${GEN_PROTO_DIR}/glyph.grpc.pb.cc"
    "${GEN_PROTO_DIR}/glyph.grpc.pb.h"
)

add_custom_command(
    OUTPUT ${GEN_SOURCES}
    COMMAND protobuf::protoc
        --proto_path="${CMAKE_CURRENT_SOURCE_DIR}/../proto"
        --cpp_out="${GEN_PROTO_DIR}"
        --grpc_out="${GEN_PROTO_DIR}"
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        "${PROTO_FILE}"
    DEPENDS "${PROTO_FILE}" protobuf::protoc gRPC::grpc_cpp_plugin
    COMMENT "Generating protobuf and gRPC sources from glyph.proto"
)

add_library(glyph_server_lib STATIC
    server/handler.cpp
    server/grpc_service.cpp
    server/http_service.cpp
    server/logger.cpp
    ${GEN_SOURCES}
)

target_include_directories(glyph_server_lib PUBLIC
    "${GEN_PROTO_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
)

target_link_libraries(glyph_server_lib PUBLIC
    Glyph::core
    Glyph::compiler_flags
    gRPC::grpc++
    protobuf::libprotobuf
    httplib::httplib
    nlohmann_json::nlohmann_json
)

add_executable(glyph_server
    main.cpp
)

target_link_libraries(glyph_server PRIVATE
    glyph_server_lib
)

if(WIN32 AND NOT MSVC)
    get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    add_custom_command(TARGET glyph_server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${COMPILER_DIR}/libstdc++-6.dll"
            "${COMPILER_DIR}/libgcc_s_seh-1.dll"
            "${COMPILER_DIR}/libwinpthread-1.dll"
            "$<TARGET_FILE_DIR:glyph_server>"
        COMMENT "Copying runtime DLLs for server execution"
    )
endif()
