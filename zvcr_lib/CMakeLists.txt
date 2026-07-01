file(GLOB_RECURSE LIBS "src/*.hpp" "src/*.h" "src/*.cpp" "src/*.c")
include_directories(zvcr ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(ZSTD_BUILD_STATIC ON)
set(ZSTD_BUILD_SHARED OFF)

include(FetchContent)
FetchContent_Declare(abseil
        GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
        GIT_TAG        master
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(abseil)

FetchContent_Declare(zstd
        URL "https://github.com/facebook/zstd/releases/download/v1.5.7/zstd-1.5.7.tar.gz"
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SOURCE_SUBDIR build/cmake
)
FetchContent_MakeAvailable(zstd)
FetchContent_Declare(fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
        GIT_TAG 12.1.0
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(fmt)
FetchContent_Declare(result
        GIT_REPOSITORY https://github.com/2b2tplace/result-cpp.git
        GIT_TAG        main
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(result)

add_library(zvcr_lib ${LIBS})

target_include_directories(zvcr_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(zvcr_lib
        PUBLIC absl::flat_hash_map
        PUBLIC result_cpp
        PUBLIC libzstd_static
        PUBLIC fmt
)
target_include_directories(zvcr_lib
        PUBLIC ${zstd_SOURCE_DIR}/lib
)