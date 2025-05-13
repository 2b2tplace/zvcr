#link_libraries(zstd)
file(GLOB_RECURSE LIBS "include/*.hpp" "include/*.h" "include/*.cpp" "include/*.c")
include_directories(zvcr_tests_run ${CMAKE_CURRENT_SOURCE_DIR}/include)

include(FetchContent)

FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
)
FetchContent_MakeAvailable(doctest)

add_executable(zvcr_tests_run ${LIBS})
target_link_libraries(zvcr_tests_run
        PRIVATE doctest
        PRIVATE zvcr_lib
)
target_include_directories(zvcr_tests_run PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(zvcr_tests_run PRIVATE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)