link_libraries(zstd)
file(GLOB_RECURSE LIBS "zvcr/*.hpp" "zvcr/*.h" "zvcr/*.cpp" "zvcr/*.c")
include_directories(zvcr ${CMAKE_CURRENT_SOURCE_DIR}/zvcr)

add_library(zvcr_lib STATIC zvcr.hpp ${LIBS})

target_compile_definitions(zvcr_lib PUBLIC PROTOCOL_VERSION=769)
target_include_directories(zvcr_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(zvcr_lib PRIVATE absl::flat_hash_map)