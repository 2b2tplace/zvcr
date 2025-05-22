# Zstd-compressed Version Controlled Region (zvcr)
A C++ library for handling the zvcr2 and zvcr3 fileformats, enabling a more feature-rich and efficient storage of Minecraft region files and top-down map data.

# What does zvcr have that mca doesn't?
The most important difference is a form of version control for region files. The zvcr3 file format stores older snapshots of data within the same region using
a reverse delta algorithm. The newest snapshot is always stored in its full form, any previous snapshot from an older time can be created by applying deltas in reverse.

Other differences include:
- the storage of the state each chunk was in when it was saved (newly generated or already existing),
- tile entity counts for each type of tile entity (useful for searching a large number of files for things like chests and shulker boxes quickly) 
- and, of course, the Zstd compression, achieving more than a 50% reduction in filesize, or a 95% reduction in the end dimension, at Zstd compression level 22.

Additionally, the zvcr2 file format was created to only store top-down information (also with reverse deltas) about a region (for map rendering or similar)
and it also includes chunk states (old/new) and tile entity counts. Any zvcr3 file can be easily converted into a zvcr2 file.

The zvcr file formats officially support Minecraft versions 1.20.4 and above.

## Include it in your project
Add the following to your CMakeLists.txt:
```cmake
include(FetchContent)

# uncomment if this repo is still private
# set(DEPLOY_KEY_PATH "~/.ssh/zvcr_repo")
# set(ENV{GIT_SSH_COMMAND} "ssh -i ${DEPLOY_KEY_PATH}")
FetchContent_Declare(zvcr
        GIT_REPOSITORY git@github.com:ESRDC/zvcr.git
        GIT_TAG        main
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(zvcr)
target_link_libraries(my_project
        PRIVATE absl::flat_hash_map
        PRIVATE zvcr_lib
)
```

# The zvcr file structure
Files as shown below are compressed with Zstd (the entire file, with a compression level of 12 by default).

## File content
| Field               | Type          | Support        | Bound                                                                                                                   |
|---------------------|---------------|----------------|-------------------------------------------------------------------------------------------------------------------------|
| zvcr Prefix         | `uint8 array` |                | Must be "ZVRegion" for zvcr3, or "ZPRegion" for zvcr2                                                                   |
| zvcr Version number | `uint8`       |                |                                                                                                                         |
| Dimension type      | `uint8`       |                |                                                                                                                         |
| Protocol version    | `uint16`      | ≥ zvcr 0.1.1.0 | See [protocol version numbers](https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge/Protocol_version_numbers) |
| Region container    |               |                |                                                                                                                         |

### Version number encoding (zvcr3)
| Version       | Version number | Supported |
|---------------|----------------|-----------|
| zvcr3 0.0.0.0 | 0              | No        |
| zvcr3 0.0.0.1 | 1              | Yes       |
| zvcr3 0.1.0.0 | 2              | Yes       |
| zvcr3 0.1.1.0 | 3              | Yes       |

### Version number encoding (zvcr2)
| Version       | Version number | Supported |
|---------------|----------------|-----------|
| zvcr2 0.0.0.0 | 0              | Yes       |
| zvcr2 0.1.0.0 | 1              | Yes       |
| zvcr2 0.1.1.0 | 2              | Yes       |
| zvcr2 0.1.1.1 | 3              | Yes       |

### Dimension type encoding
| Dimension type | Number | Section count |
|----------------|--------|---------------|
| Overworld      | 0      | 24            |
| Nether         | 1      | 16            |
| The End        | 2      | 16            |

### Region container
| Field                  |
|------------------------|
| Palette table          |
| 1024 Optional Segments |

## Paletted storage
Block state and biome data is compressed per snapshot by packing the data using palettes, similar to the mca file format; 
however, zvcr does not have single value palettes implemented.

### Palette table
| Field                  | Type     |
|------------------------|----------|
| Palette table length n | `uint32` |
| n Palettes             |          |

### Palette
| Field             | Type                         |
|-------------------|------------------------------|
| Palette length n  | `uint16`                     |
| Palette data      | `uint16 array` with length n |

### Packed delta data
| Field              | Type     |
|--------------------|----------|
| Delta length n     | `uint64` |
| n Packed snapshots |          |

### Packed snapshot
A snapshot represents a batch of a palette packed delta data snapshot with a given fixed `snapshot size` when unpacked.
The latest snapshot (the first one read in delta data) contains all data in this snapshot. All snapshots after it are represented with reverse deltas, which allows
for the full recreation of older data by applying deltas on top of the latest snapshot. 

Unpacked data is stored as a `uint16 array`. Unchanged entries, when unpacked, are represented with `0xFFFF`.
Packed data is stored as packed `uint64 array` with `snapshot size` palette entry indices, all being the same length; the minimum number of bits required to
represent the largest index in the palette:
```cpp
uint64_t getBitsPerIndex(size_t paletteSize) {
    return max(int(ceil(log2(paletteSize))), 1);
}
```

See [the implementation of paletted data storage](/zvcr_lib/zvcr/common/data_storage.cpp) for more details.

Packed snapshots are formatted as such:

| Field           | Type                                        |
|-----------------|---------------------------------------------|
| Timestamp       | `uint64`                                    |
| Packed length n | `uint64`                                    |
| Packed data     | `uint64 array` with length n                |
| Palette index   | `uint32` (index in the given palette table) |

## Segments
Segments describe a Minecraft chunk embedded within the region (16 blocks in sidelength). By their nature, they can be absent if they were not stored in their position.

### Optional Segment
| Field             | Type      | Note                                              |
|-------------------|-----------|---------------------------------------------------|
| Segment indicator | `boolean` | `uint8`, zero representing false                  |
| Segment           |           | Only present if the segment indicator was nonzero |
| Segment info      |           | Only present if the segment indicator was nonzero |

### Segment (zvcr3)
A segment in zvcr3 extends vertically and consists of n block and biome sections (n depending on the dimension type).

| Field            | Type                                                       | Support         |
|------------------|------------------------------------------------------------|-----------------|
| n Block sections | Packed delta data with `snapshot length` = 16x16x16 = 4096 |                 |
| n Biome sections | Packed delta data with `snapshot length` = 4x4x4 = 64      | ≥ zvcr3 0.1.0.0 |

### Segment (zvcr2)
A segment in zvcr2 describes several layers of top-down block and biome data.

| Field           | Type                                          | Support         |
|-----------------|-----------------------------------------------|-----------------|
| Layers length n | `uint64`                                      |                 |
| n Block layers  | Layer with `snapshot length` = 16x16x16 = 256 |                 |
| n Biome layers  | Layer with `snapshot length` = 4x4 = 16       | ≥ zvcr2 0.1.0.0 |

### Layer
A layer can describe block or biome information and as such has a fixed given snapshot length.

| Field             | Type                                           |
|-------------------|------------------------------------------------|
| Layer type id     | `uint8`                                        |
| Packed delta data | Packed delta data with given `snapshot length` |

### Layer type id encoding
| Layer type name   | Id      | Note                                                                                               |
|-------------------|---------|----------------------------------------------------------------------------------------------------|
| Normal            | 0       | Top down view of all blocks                                                                        |
| Terrain           | 1       | Top down view of terrain                                                                           |
| Heightmap         | 2       | Y levels of each block within the Normal layer                                                     |
| Terrain Heightmap | 3       | Y levels of each block within the Terrain layer                                                    |
| Drained           | 4       | Top down view of blocks, excluding liquids                                                         |
| Drained Heightmap | 5       | Y levels of each block within the Drained layer                                                    |
| Predicted         | 6       | Used for post-addition of biomes in older zvcr files before biomes were implemented                |
| Custom            | 7...255 | Custom layer ids used should start at high numbers, as new reserved layers are incrementally added |

### Segment info
Additional segment info is stored across both zvcr2 and zvcr3 for miscellaneous applications. These include visibly seeing which chunks on a Minecraft server
were newly generated or not. Along with that, a more efficient tile entity counts storage to quickly filter for treasures when scanning large amounts of data.

| Field                     | Type     |
|---------------------------|----------|
| Segment states length n   | `uint64` |
| n Segment states          |          |
| Tile entities length k    | `uint64` |
| k Tile entity counts info |          |

### Segment state
| Field         | Type     |
|---------------|----------|
| State type id | `uint8`  |
| Timestamp     | `uint64` |

### Segment state type id encoding
| State type name | Id |
|-----------------|----|
| Unknown         | 0  |
| New             | 1  |
| Old             | 2  |

### Tile entity counts info
| Field              | Type           |
|--------------------|----------------|
| Tile entity counts | `uint16 array` |
| Timestamp          | `uint64`       |

No length is specified in the tile entity counts info, since the length is entirely dictated by the used protocol
version and how many tile entity types exist in that Minecraft version. The array is ordered by tile entity type id and each entry 
represents the number of that tile entity present here.

# What are zvr and zpr files?
The older name of this file format and library was previously `libzr` with `zvr` (Zstd-compressed Voxel Region) being the old `zvcr3` and `zpr`
(Zstd-compressed Pixel Region) being the old `zvcr2`. The older file formats should remain compatible with the new library. New names for these things were
chosen mostly for consistency both in the code and the general naming of everything. The `ZVRegion` and `ZPRegion` header prefixes are still used for backwards compatibility.
