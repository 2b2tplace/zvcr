cmake_minimum_required(VERSION 3.29)
project(zvcr)

set(CMAKE_CXX_STANDARD 20)

link_libraries(zstd)
file(GLOB_RECURSE LIBS "include/*.hpp" "include/*.h" "include/*.cpp" "include/*.c")
include_directories(zvcr ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(zvcr INTERFACE ${LIBS})
target_compile_definitions(zvcr INTERFACE ZVCR_VERSION=ZVCR_0_0_0_0)
target_include_directories(zvcr INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)