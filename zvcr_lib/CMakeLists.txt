link_libraries(zstd)
file(GLOB_RECURSE LIBS "*.hpp" "*.h" "*.cpp" "*.c")
include_directories(zvcr ${CMAKE_CURRENT_SOURCE_DIR})

set(DEPLOY_KEY_PATH "~/.ssh/result_repo")
set(ENV{GIT_SSH_COMMAND} "ssh -i ${DEPLOY_KEY_PATH}")
FetchContent_Declare(result
        GIT_REPOSITORY git@github.com:2b2tplace/result-cpp.git
        GIT_TAG        main
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(result)

add_library(zvcr_lib ${LIBS})

target_include_directories(zvcr_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(zvcr_lib
        PUBLIC absl::flat_hash_map
        PUBLIC result_cpp
)