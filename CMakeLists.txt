cmake_minimum_required(VERSION 3.29)
project(libzr)

set(CMAKE_CXX_STANDARD 23)

link_libraries(zstd)
file(GLOB_RECURSE LIBS "include/*.hpp" "include/*.h" "include/*.cpp" "include/*.c")

add_library(libzr STATIC ${LIBS})
target_include_directories(libzr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)