cmake_minimum_required(VERSION 3.29)
project(zvcr)

set(CMAKE_CXX_STANDARD 20)
set(SOURCE_FILES main.cpp)
add_executable(zvcr_run ${SOURCE_FILES})

include_directories(zvcr_lib)
add_subdirectory(zvcr_lib)

target_link_libraries(zvcr_run zvcr_lib)

add_subdirectory(zvcr_tests)