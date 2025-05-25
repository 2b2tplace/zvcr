link_libraries(zstd)
file(GLOB_RECURSE LIBS "*.hpp" "*.h" "*.cpp" "*.c")
include_directories(zvcr ${CMAKE_CURRENT_SOURCE_DIR})

add_library(zvcr_lib ${LIBS})

target_include_directories(zvcr_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(zvcr_lib PRIVATE absl::flat_hash_map)