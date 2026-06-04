cmake_minimum_required(VERSION 3.20)
project(Glyph VERSION 0.1.0 LANGUAGES CXX)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/CompilerFlags.cmake)
include(cmake/Dependencies.cmake)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(bench)
