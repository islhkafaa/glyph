include(FetchContent)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
set(OPENSSL_NO_ASM ON CACHE BOOL "" FORCE)
set(HTTPLIB_HEADER_ONLY ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann_json-src
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/third_party/nlohmann_json-build
)

FetchContent_Declare(
  cpp-httplib
  URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.14.3.zip
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cpp-httplib-src
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/third_party/cpp-httplib-build
)

find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf REQUIRED)

FetchContent_MakeAvailable(nlohmann_json cpp-httplib)
