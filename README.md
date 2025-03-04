# Zstd-compressed Version Controlled Region (zvcr)
A C++ library for handling the zvcr2 and zvcr3 fileformats, enabling a more feature-rich and efficient storage of Minecraft region files and top-down map data.

# What does zvcr have that mca doesn't?
The most important difference is a form of version control for region files. The zvcr3 file format stores older snapshots of data within the same region using
a reverse delta algorithm (the newest snapshot is always stored in its full form, any previous snapshot from an older time can be created by applying deltas in reverse).
Other differences include the storage of the state each chunk was in when it was saved (newly generated or already existing) along with timestamps for these states,
tile entity counts for each type of tile entity (useful for searching a huge amount of files for things like chests and shulker boxes) and, of course, the Zstd 
compression, achieving more than a 50% reduction in filesize, or a 95% reduction in the end dimension, at Zstd compression level 22.

Additionally, the zvcr2 file format was created to only store top-down information (also with reverse deltas) about a region (for map rendering or similar)
and it also includes chunk states (old/new) and tile entity counts. Any zvcr3 file can be easily converted into a zvcr2 file.

# The basic zvcr3 file structure
The format compresses block state information by packing the data exactly how mca would do it (See https://minecraft.wiki/w/Region_file_format), but does not 
have single value palettes implemented. The entire file as shown below is compressed with Zstd.
```text
"ZVRegion" (File prefix literal)
Version Number
Dimension Type Number
32 x 32 (512) Optional Region Segments
├── Optional Segment (0 byte if there is no region), otherwise:
    ├── Segment (16x384x16 blocks in overworld, 16x256x16 in nether/end)
        ├── n Segment Sections (24 in overworld, 16 in nether/end)
            ├── Length (total snapshots)
            ├── Block States Snapshots (first in full, subsequent snapshots are reverse deltas)
                ├── Snapshot Unix Timestamp 
                ├── Packed Data Length
                ├── Packed Data
                ├── Palette Index (used for unpacking the data)
            ├── Biome Snapshots (instead of 16x16x16 blockstates, only 4x4x4 biomes)
                ├── [same as Block States Snapshots, see above]
        ├── Segment Info
            ├── Segment States Length
            ├── Segment States
                ├── State Id
                ├── Snapshot Unix Timestamp
            ├── Tile Entity Counts Length
            ├── Tile Entity Counts
                ├── Counts Array (in the order of tile entities defined in zvcr)
                ├── Snapshot Unix Timestamp
Palette Table
├── Palette Table Length
├── Palettes
    ├── Palette Length
    ├── Palette Data
```

# The basic zvcr2 file structure
This format operates very similarly to the zvcr3 format, except it does not store entire chunks of data, but only top-down layers for future rendering
of a world map or other uses such as scanning the world for specific tile entities way faster than achievable in zvcr3.
```text
"ZPRegion" (File prefix literal)
Version Number
Dimension Type Number
32 x 32 (512) Optional Region Segments
├── Optional Segment (0 byte if there is no region), otherwise:
    ├── Layers Length
    ├── Layers
        ├── Layer Type
        ├── Layers Snapshots (16x16 blockstates)
            ├── Snapshot Unix Timestamp 
            ├── Packed Layer Data Length
            ├── Packed Layer Data
            ├── Palette Index
        ├── Biome Layers Snapshots (4x4 biomes)
            ├── [same as Layers Snapshots, see above]
    ├── Segment Info
        ├── [same as in zvcr3, see above]
Palette Table
├── [same as in zvcr3, see above]
```
zvcr2 layers can hold various kinds of information. The basic layer types are:

| Name                        | Id      |
|-----------------------------|---------|
| Top-down                    | 0       |
| Top-down (no roof)          | 1       |
| Heightmap                   | 2       |
| Heightmap (no roof)         | 3       |
| Top-down (drained)          | 4       |
| Top-down (drained, no roof) | 5       |
| Custom                      | 6...255 |

Each layer stores a 16x16 block area of unsigned 16-bit integers for each segment in the region file. This data can be interpreted in multiple ways,
such as block states for the top-down layer types, or a height value for heightmap layers, but also any other kind of information when using custom layers.
Layers are used to achieve special rendering features such as semi-transparent water when rendered in a world map, or ignoring snow or large areas of obsidian.

# What are zvr and zpr files?
The older name of this file format and library was previously `libzr` with `zvr` (Zstd-compressed Voxel Region) being the old `zvcr3` and `zpr`
(Zstd-compressed Pixel Region) being the old `zvcr2`. The older file formats should remain compatible with the new library. New names for these things were
chosen mostly for consistency both in the code and the general naming of everything

## Include it in your project
Simply add the following to your CMakeLists.txt:
```cmake
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

# other FetchContent_Declare declarations

# uncomment below if this repo is not public yet
# create a deploy key for this repo on github
## set(DEPLOY_KEY_PATH "~/.ssh/zvcr_repo")
## set(ENV{GIT_SSH_COMMAND} "ssh -i ${DEPLOY_KEY_PATH}")
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
