# Zstd-compressed Version Controlled Region (ZVCR)
A C++ library for handling the zvcr2 and zvcr3 fileformats, enabling a more feature-rich and efficient storage of Minecraft region files and top-down map data.

## Include it in your project
Simply add the following to your CMakeLists.txt:
```cmake
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

# other FetchContent_Declare declarations
FetchContent_Declare(zvcr
        GIT_REPOSITORY git@github.com:ESRDC/zvcr.git
        GIT_TAG        main
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
)
FetchContent_GetProperties(zvcr)
if(NOT zvcr_POPULATED)
    FetchContent_MakeAvailable(zvcr)
endif()

target_link_libraries(MyProject
        # other libraries
        PRIVATE zvcr
)
```
