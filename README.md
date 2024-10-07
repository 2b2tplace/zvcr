# libzr
A C++ library for the ZR file formats: ZVR (Zstd-compressed Voxel Region) and ZPR (Zstd-compressed Pixel Region) for storing Minecraft region files in a more optimized manner.

## Include it in your project
Simply add the following to your CMakeLists.txt:
```cmake
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

# other FetchContent_Declare declarations
FetchContent_Declare(libzr
        GIT_REPOSITORY git@github.com:Enclave-Science-Centre/libzr.git
        GIT_TAG        main
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
)
FetchContent_GetProperties(libzr)
if(NOT libzr_POPULATED)
    FetchContent_MakeAvailable(libzr)
endif()

target_link_libraries(MyProject
        # other libraries
        PRIVATE libzr
)
```
