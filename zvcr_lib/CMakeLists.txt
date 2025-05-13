link_libraries(zstd)
file(GLOB_RECURSE LIBS "zvcr/*.hpp" "zvcr/*.h" "zvcr/*.cpp" "zvcr/*.c")
include_directories(zvcr ${CMAKE_CURRENT_SOURCE_DIR}/zvcr)

include(FetchContent)

FetchContent_Declare(
        visit_struct
        GIT_REPOSITORY https://github.com/cbeck88/visit_struct.git
)
FetchContent_MakeAvailable(visit_struct)

FetchContent_Declare(
        magic_enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
)
FetchContent_MakeAvailable(magic_enum)

add_subdirectory(abseil-cpp)
add_library(zvcr_lib STATIC zvcr.hpp ${LIBS})

target_compile_definitions(zvcr_lib PUBLIC PROTOCOL_VERSION=769)
target_include_directories(zvcr_lib
        PUBLIC ${visit_struct_SOURCE_DIR}/include
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(zvcr_lib
        PUBLIC magic_enum
        PRIVATE absl::flat_hash_map
)