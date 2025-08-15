cmake_minimum_required(VERSION 3.25)
project(zvcr)

set(CMAKE_CXX_STANDARD 23)
set(SOURCE_FILES main.cpp)
add_executable(zvcr_run ${SOURCE_FILES})

include(FetchContent)
FetchContent_Declare(abseil
        GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
        GIT_TAG        master
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(abseil)

include_directories(zvcr_lib)
add_subdirectory(zvcr_lib)

target_link_libraries(zvcr_run zvcr_lib)

add_subdirectory(zvcr_tests)