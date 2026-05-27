# Zstd-compressed Version Controlled Region (zvcr)
A C++ library for handling the zvcr2 and zvcr3 file formats, enabling a more feature-rich and efficient storage of 
Minecraft region files and top-down map data, in a Minecraft Java Edition large-scale world data archival context.

# What does zvcr have that mca doesn't?
The most important difference is a form of version control for region files. The zvcr3 file format stores older snapshots
of data within the same region using a reverse delta algorithm. The newest snapshot is always stored in its full form,
any previous snapshot from an older time can be created by applying deltas in reverse.

Other differences include:
- the storage of the state each chunk was in when it was saved (newly generated or already existing),
- Zstd compression, achieving more than a 50% reduction in filesize, or a 95% reduction in the end dimension, at Zstd 
  compression level 22.

Additionally, the zvcr2 file format was created to only store top-down information (also with reverse deltas) about a 
region (for map rendering or similar) and it also includes chunk states (old/new). Any zvcr3 file can be easily flattened
into a zvcr2 file.

The zvcr file formats officially support Minecraft versions 1.20.4 and above.

# Why?
We wanted a long-term solution to keep adding new features and ideas to compress world data even more. One of those ideas
was delta storage, but this won't be the only benefit of this file format. We are open to ideas to make this even more 
efficient and feature-rich.

## Include it in your project
Add the following to your CMakeLists.txt:
```cmake
include(FetchContent)

FetchContent_Declare(zvcr
        GIT_REPOSITORY git@github.com:2b2tplace/zvcr.git
        GIT_TAG main
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(zvcr)
target_link_libraries(my_project
        PRIVATE absl::flat_hash_map
        PRIVATE zvcr_lib
)
```

# The zvcr File Structure
TODO: Explain the zvcr directory structure when dealing with a world save (collection of region files).

## Compression
Files as shown below are compressed with [Zstd](https://github.com/facebook/zstd). The entire file\* is compressed, with a
Zstd level of 10 by default (compared to the Anvil region format traditionally compressing individual chunks). The decision
to compress the entire file instead of individual chunks was made specifically to benefit the compression ratio. Higher 
levels of Zstd can still bring down the file size of zvcr region files by a significant amount, however these will come 
with greater performance losses.

The zvcr file format was never intended for use in a full Minecraft server, which actively reads and writes chunks on 
demand, and was instead created purely for long term data archival. The side effects of this (in memory usage) when used
in a world downloader server, or in the PlaceViewer server are minimal.

Testing of region-level Zstd compression has also proven successful in the [Linear region format](https://github.com/xymb-endcrystalme/LinearRegionFileFormatTools), 
seeing similar benefits, even though that file format was created for use in running actual Minecraft servers.

\*There are plans to exclude the header from compression, and only compressing the Region container. This change will be
implemented before release 1.0.0.0 of zvcr.

## Endianness
Most of zvcr is explicitly stored in little-endian. The only exception is NBT data found in tile entities, which is 
serialized in big-endian. This decision was made purely for a standard NBT implementation that can easily interoperate
with Minecraft.

## File Content
| Field               | Type                                                                                                                              | Support                           | Bound                                                 |
|---------------------|-----------------------------------------------------------------------------------------------------------------------------------|-----------------------------------|-------------------------------------------------------|
| zvcr Prefix         | `uint8 array` with length 8 (fixed-size `string` without a length prefix)                                                         |                                   | Must be "ZVRegion" for zvcr3, or "ZPRegion" for zvcr2 |
| zvcr Version number | `uint8`, see [Version Numbers](#version-numbers)                                                                                  |                                   |                                                       |
| Dimension Type      | `uint8`, see [Dimension Type Encoding](#dimension-type-encoding)                                                                  |                                   |                                                       |
| Protocol Version\*  | `uint16`, see [Protocol Version Numbers](https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge/Protocol_version_numbers) | ≥ zvcr3 0.1.1.0 / ≥ zvcr2 0.1.1.0 |                                                       |
| Region Container    | [Region Container](#region-container)                                                                                             |                                   |                                                       |

\*The Protocol Version is used to determine which registries of the game are used. This importantly dictates which block
state IDs (note: not block IDs, but specifically numeric block state IDs), biome type IDs and tile entity IDs/NBT formats
to use. Whenever the zvcr documentation states block state ID, biome type ID, or tile entity ID, encoded as an unsigned
integer, that numeric ID refers to the particular entry with that ID in the registry of the given context. Registries can
be generated using [data generators](https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge/Data_Generators),
extracting information from Minecraft server/client JAR files. These numeric IDs usually are not found explicitly within
Minecraft source code, and are rather implied from the order the entries were defined in. Furthermore, biome type IDs
only refer to default biomes as provided on a vanilla Java Edition Minecraft server

The version being specified at the region-level instead of within individual chunks prevents version mismatches within 
a single zvcr world save. This implies partially upgrading some chunks but not others in zvcr is not possible, and instead 
the entire world must be upgraded to a newer Minecraft version. A one-time upgrading operation is more beneficial for 
consistency and performance, compared to upgrading on an as-needed basis, especially when handling zvcr files in a 
read-only context.

## Version Numbers
This implementation has gone through multiple iterations of zvcr. When version 1.0.0.0 of zvcr releases, support for
older zvcr files will be fully dropped (as they have only seen use internally, with files of such old versions never 
distributed to the public).

### Version Number Encoding (zvcr3)
| Version       | Version number | Supported |
|---------------|----------------|-----------|
| zvcr3 0.0.0.0 | 0              | No        |
| zvcr3 0.0.0.1 | 1              | Yes       |
| zvcr3 0.1.0.0 | 2              | Yes       |
| zvcr3 0.1.1.0 | 3              | Yes       |

### Version Number Encoding (zvcr2)
| Version       | Version number | Supported |
|---------------|----------------|-----------|
| zvcr2 0.0.0.0 | 0              | Yes       |
| zvcr2 0.1.0.0 | 1              | Yes       |
| zvcr2 0.1.1.0 | 2              | Yes       |
| zvcr2 0.1.1.1 | 3              | Yes       |

### Dimension Type Encoding
The world height, and consequently the number of chunk sections, depends on the dimension type. This is currently 
hard-coded for each of the vanilla Minecraft dimension types, but support for modded dimensions is planned. 

### Unsigned Y Levels
Importantly, Y levels defined all throughout zvcr are unsigned, with Y = 0 always being the bottom of the world. 
Converting Y levels between the base game and zvcr is trivial (zvcr_y + min_y = minecraft_y). This was an intentional
design choice to make handling zvcr files alone a little easier. The potential for confusion only appears in the
event when writing code that handles converting zvcr and Minecraft Y levels.

| Dimension type | Number | Section count | Minimum Block Y (Minecraft) | World Block Height\* |
|----------------|--------|---------------|-----------------------------|----------------------|
| Overworld      | 0      | 24            | -64                         | 384                  |
| Nether         | 1      | 16            | 0                           | 256                  |
| The End        | 2      | 16            | 0                           | 256                  |

\*World Block Height here refers to the actual height of the world in blocks (section count \* 16), and not the maximum 
block Y level in Minecraft (which is 320 = 384 + (-64) in the Overworld for instance).

### Region container
| Field                  | Type                                                              |
|------------------------|-------------------------------------------------------------------|
| Palette Table          | [Palette Table](#palette-table)                                   |
| 1024 Optional Segments | `array` of [Optional Segment](#optional-segment) with length 1024 |

## Paletted storage
Block state and biome data is compressed per snapshot by packing the data using palettes, similar to the mca file format.

### Palette Table
| Field                  | Type                                         |
|------------------------|----------------------------------------------|
| Palette Table Length n | `uint32`                                     |
| n Palettes             | `array` of [Palette](#palette) with length n |
Direct and single-value palettes are not stored in this table; See [Direct Palettes](#direct-palettes) for more info.

### Palette
| Field            | Type                         |
|------------------|------------------------------|
| Palette Length n | `uint16`                     |
| Palette Data     | `uint16 array` with length n |

## Segments
Segments describe a Minecraft chunk embedded within the region (16 blocks in sidelength). By their nature, they can be absent if they were not stored in their position.

### Optional Segment
| Field               | Type                                        | Note                                                                 | Support         |
|---------------------|---------------------------------------------|----------------------------------------------------------------------|-----------------|
| Segment Indicator   | `boolean`                                   | `uint8`, zero indicating the absence of this segment                 |                 |
| Segment             | [Segment](#segment)                         | Only present if the segment indicator was nonzero                    |                 |
| Segment Info        | [Segment Info](#segment-info)               | Only present if the segment indicator was nonzero                    |                 |
| Tile Entity History | [Tile Entity History](#tile-entity-history) | Only present in zvcr3, and only if the segment indicator was nonzero | ≥ zvcr3 0.1.4.0 |

## Segment
### Segment (zvcr3)
A segment in zvcr3 extends vertically and consists of n block and biome sections (n depending on the dimension type).

| Field            | Type                                                                             | Support         |
|------------------|----------------------------------------------------------------------------------|-----------------|
| n Block Sections | [Packed Delta Data](#packed-delta-data) with `snapshot length` = 16x16x16 = 4096 |                 |
| n Biome Sections | [Packed Delta Data](#packed-delta-data) with `snapshot length` = 4x4x4 = 64      | ≥ zvcr3 0.1.0.0 |

### Segment (zvcr2)
A segment in zvcr2 describes several layers of top-down block and biome data.

| Field           | Type                                                 | Support         |
|-----------------|------------------------------------------------------|-----------------|
| Layers Length n | `uint64`                                             |                 |
| n Block Layers  | [Layer](#layer) with `snapshot length` = 16x16 = 256 |                 |
| n Biome Layers  | [Layer](#layer) with `snapshot length` = 4x4 = 16    | ≥ zvcr2 0.1.0.0 |

### Packed Delta Data
| Field              | Type                                                         |
|--------------------|--------------------------------------------------------------|
| Delta Length n     | `uint64`                                                     |
| n Packed Snapshots | `array` of [Packed Snapshot](#packed-snapshot) with length n |

### Packed Snapshot
A snapshot represents a batch of a palette packed delta data snapshot with a given fixed `snapshot length` when unpacked.
The latest snapshot (the first one read in delta data) contains all data in this snapshot. All snapshots after it are represented with reverse deltas, which allows
for the full recreation of older data by applying deltas on top of the latest snapshot.

Unpacked data is stored as a `uint16 array`. Unchanged entries, when unpacked, are represented with `0xFFFF`.
Packed data is stored as packed `uint64 array` with `snapshot length` palette entry indices, all being the same length; the minimum number of bits required to
represent the largest index in the palette:
```cpp
uint64_t getBitsPerIndex(size_t paletteSize) {
    return max(bit_width(max(paletteSize, 1UL) - 1), 1UL);
}
```

#### Direct Palettes
If the bits per index of a palette exceeds 8, storing the packed data + the palette results in actually storing more bytes.
To combat this, direct mode is used for any palette with bits per index > 8.
Direct mode creates a 1:1 mapping of `uint16_t` entries and avoids palette usage altogether.
The entries are still packed in a `uint64 array` and the bits per index is hard coded to 16 in this case.

See [the implementation of paletted data storage](/zvcr_lib/src/zvcr/common/data_storage.cpp) for more details.

Packed snapshots are formatted as such:

| Field           | Type                                                                                                                             |
|-----------------|----------------------------------------------------------------------------------------------------------------------------------|
| Timestamp       | `uint64` (Unix time, seconds)                                                                                                    |
| Palette Type    | `uint8`, = 0 indicates a single-value palette, = 1 indicates section palette                                                     |
| Palette Value   | `uint16`, only present if single-value palette was used                                                                          |  
| Packed Length n | `uint64`, only present if section palette was used                                                                               |
| Packed Data     | `uint64 array` of length n, only present if section palette was used                                                             |
| Palette Index   | `uint32`, only present if section palette was used, index in the given palette table, `UINT32_MAX` to encode direct palette mode |

### Layer
A layer can describe block or biome information and as such has a fixed given `snapshot length`.

| Field             | Type                                                                 |
|-------------------|----------------------------------------------------------------------|
| Layer Type ID     | `uint8` (See [Layer Type ID Encoding](#layer-type-id-encoding))      |
| Packed Delta Data | [Packed Delta Data](#packed-delta-data) with given `snapshot length` |

### Layer Type ID Encoding
| Layer type name   | ID      | Note                                                                                                                                             |
|-------------------|---------|--------------------------------------------------------------------------------------------------------------------------------------------------|
| Normal            | 0       | Top down view of all blocks                                                                                                                      |
| Terrain           | 1       | Top down view of terrain                                                                                                                         |
| Heightmap         | 2       | Y levels of each block within the Normal layer                                                                                                   |
| Terrain Heightmap | 3       | Y levels of each block within the Terrain layer                                                                                                  |
| Drained           | 4       | Top down view of blocks, excluding liquids                                                                                                       |
| Drained Heightmap | 5       | Y levels of each block within the Drained layer                                                                                                  |
| ~~Predicted~~     | ~~6~~   | ~~Used for post-addition of biomes in older zvcr files before biomes were implemented~~ Deprecated, will be removed upon release 1.0.0.0 of zvcr |
| Custom            | 7...255 | Custom layer ids used should start at high numbers, as new reserved layers are incrementally added                                               |

### Segment Info
Additional segment info is stored across both zvcr2 and zvcr3 for miscellaneous applications. These include visibly seeing which chunks on a Minecraft server
were newly generated or not. Along with that, a more efficient tile entity counts storage to quickly filter for treasures when scanning large amounts of data.

| Field                         | Type                                                     | Support                                             |
|-------------------------------|----------------------------------------------------------|-----------------------------------------------------|
| Segment States Length n       | `uint64`                                                 |                                                     |
| n Segment States              | `array` of [Segment State](#segment-state) with length n |                                                     |
| ~~Tile Entities Length k~~    | `uint64`                                                 | ≤ zvcr3 0.1.2.0 / ≤ zvcr2 0.1.3.0 (Support removed) |
| ~~k Tile Entity Counts Info~~ |                                                          | ≤ zvcr3 0.1.2.0 / ≤ zvcr2 0.1.3.0 (Support removed) |

### Segment state
| Field         | Type                                                                           |
|---------------|--------------------------------------------------------------------------------|
| State Type ID | `uint8` (See [Segment State Type ID Encoding](#segment-state-type-id-encoding) |
| Timestamp     | `uint64` (Unix time, seconds)                                                  |

### Segment State Type ID Encoding
| State type name | ID |
|-----------------|----|
| Unknown         | 0  |
| New             | 1  |
| Old             | 2  |

### Tile Entity History
| Field                       | Type                                                                                    |
|-----------------------------|-----------------------------------------------------------------------------------------|
| Delta Length n              | `uint64`                                                                                |
| n Tile Entity List Snapshot | `array` of [Tile Entity List Snapshot](#tile-entity-list-snapshot) with length n (\*\*) |

### Tile Entity List Snapshot
| Field                     | Type                                            |
|---------------------------|-------------------------------------------------|
| Timestamp                 | `uint64` (Unix time, seconds)                   |
| Tile Entity List Length n | `uint64`                                        |
| n Tile Entities           | `array` of [Tile Entity](#tile-entity) length n |

### Tile Entity
| Field             | Type                      | Note                                            |
|-------------------|---------------------------|-------------------------------------------------|
| Packed Position   | `uint32`                  | Calculated as below (\*)                        |
| Operation         | `uint8`                   | 1 indicating "put", 0 indicating "erase" (\*\*) |
| Tile Entity Type  | `uint32`                  | Only present if Operation was nonzero           |
| NBT Data Length n | `uint64`                  | Only present if Operation was nonzero           |
| NBT Data Buffer   | `uint8 array` of length n | Only present if Operation was nonzero. (\*\*\*) |

(\*) The packed position consists out of the local x/z coordinates (both internally stored as `uint8`) of the tile 
entity block within the chunk, and the y coordinate (internally as `uint16`) ranging from 0 to full world block height.
These coordinates can be packed and unpacked as such:
```cpp
    struct TileEntityPosition {
        uint8_t x;
        uint8_t z;
        uint16_t y;
    };

    uint32_t getPackedPosition(TileEntityPosition pos) {
        return static_cast<uint32_t>(pos.y) << 16
            | static_cast<uint32_t>(pos.z) << 8
            | static_cast<uint32_t>(pos.x);
    }

    TileEntityPosition unpack(uint32_t packedPosition) {
        return TileEntityPosition {
            .x = static_cast<uint8_t>(packedPosition & 0xFF),
            .z = static_cast<uint8_t>(packedPosition >> 8 & 0xFF),
            .y = static_cast<uint16_t>(packedPosition >> 16)
        };
    }
```

(\*\*) As tile entities are also stored as a history of delta snapshots, similar to block and biome data, there is a distinction 
between inserting and erasing tile entities in snapshots. As confusing as it may be, this also happens in reverse.
The latest snapshot includes all tile entities listed out using the 'put' Operation. If a given tile entity did not exist 
yet at an earlier point in time, the tile entity in that older snapshot will be assigned the 'erase' Operation. The
naming convention here may be subject to change, as it requires thinking in reverse time. If a tile entity did not change
(NBT data buffer is the same), the tile entity is excluded from the delta snapshot.

More Operations are planned, where individual NBT tags could be modified, to save on storage.

(\*\*\*) NBT data is stored as a raw byte buffer following the
[NBT specification, as seen on the Minecraft wiki](https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge/NBT#Specification).
Specifically, the NBT dialect from the Java Edition of Minecraft (1.20.4+) is used, where data is stored in big endian.
An important distinction here is the use of [modified UTF-8](https://docs.oracle.com/javase/8/docs/api/java/io/DataInput.html#modified-utf-8)
for strings, as simply writing out strings in regular UTF-8 is not supported by the Java Edition of Minecraft. Using
modified UTF-8 is especially necessary when encountering Unicode codepoints in the range U+10000 to U+10FFFF which are 
traditionally stored in 4 bytes in UTF-8, but are stored using 6 bytes in modified UTF-8. Disregarding this requirement
will result in Java Edition clients disconnecting from the server, when receiving NBT data with strings wrongly encoded 
in UTF-8 instead of modified UTF-8. See below for the code snippet where this would become an obvious issue.

The NBT format used for tile entities in particular follows the same structure as when sent over the network. To be more
specific, tile entity NBT in zvcr does not contain the `id`, `keepPacked`, `x`, `y`, and `z` NBT tags that are found in
tile entity NBT used in Anvil region files. Instead of NBT tags, the `Tile entity type` (replacing `id`) and `Packed position`
(replacing `x`, `y`, `z`) fields are used here. `keepPacked` is a special NBT tag used in the base game to differentiate 
invalid tile entities in Anvil region files, and is not present in zvcr. Serialized NBT in zvcr files can be directly used
during the construction of packets, without the need to (de)serialize an of the NBT data. This is a consequence of storing
tile entities as-is when received from a Java Edition Minecraft server.

Example from PlaceViewer, where tile entities are read directly from a zvcr region file and then sent in a Chunk Data 
and Update Light packet (`buffer` in this code snippet is the packet buffer, which is being written into):
```cpp
const auto &tileEntities = segment->tileEntities.snapshotFrom(timestamp).value_or(zvcr::TileEntityList{});
pc::WriteData<pc::VarInt>(static_cast<int32_t>(tileEntities.size()), buffer);
for (const auto &[pos, tileEntity] : tileEntities) {
    pc::WriteData<uint8_t>(static_cast<uint8_t>((pos.x & 15) << 4 | pos.z & 15), buffer);
    pc::WriteData<int16_t>(static_cast<int16_t>(pos.y + minY), buffer);
    pc::WriteData<pc::VarInt>(static_cast<int32_t>(tileEntity.type), buffer);
    
    // Directly insert the NBT data from zvcr into the Data field of the tile entity.
    buffer.insert(buffer.end(), tileEntity.nbt.begin(), tileEntity.nbt.end());
}
```

In zvcr, NBT tags are always sorted alphabetically by their keys when found in NBT tag compounds. This alphabetical 
sorting should occur before serializing NBT to a byte buffer, when passed into a 
[zvcr::TileEntity](zvcr_lib/src/zvcr/region/segment/tile_entities.hpp) structure. Sorting NBT keys this way is currently a requirement, as this zvcr implementation does
not include NBT (de)serialization, and adding NBT comparisons that ignore key order would be more expensive, compared to
directly checking NBT byte buffers for equality. Disregarding this requirement will result in tile entity deltas being 
wrongly created, due to serialized NBT byte buffers being different, despite the underlying NBT data still being the same.
This is not necessarily a big issue, but it will inflate the filesize. This is a temporary fix in the current 
implementation of zvcr, and will likely be fixed by including an NBT library and doing proper NBT compound comparisons. 

The NBT data buffer also remains uncompressed (may change in the future, signaling this by using a different Operation number).

### ~~Tile Entity Counts Info~~ (deprecated, only exists for ≤ zvcr3 0.1.2.0 / ≤ zvcr2 0.1.3.0)
| Field              | Type                           |
|--------------------|--------------------------------|
| Tile Entity Counts | `uint16 array`                 |
| Timestamp          | `uint64` (Unix time, seconds)  |

~~No length is specified in the tile entity counts info, since the length is entirely dictated by the used protocol~~
~~version and how many tile entity types exist in that Minecraft version. The array is ordered by tile entity type id and each entry~~ 
~~represents the number of that tile entity present here.~~

# ~~What are zvr and zpr files?~~ (deprecated, will be removed for zvcr 1.0.0.0)
~~The older name of this file format and library was previously `libzr` with `zvr` (Zstd-compressed Voxel Region) being the old `zvcr3` and `zpr`~~
~~(Zstd-compressed Pixel Region) being the old `zvcr2`. The older file formats should remain compatible with the new library. New names for these things were~~
~~chosen mostly for consistency both in the code and the general naming of everything. The `ZVRegion` and `ZPRegion` header prefixes are still used for backwards compatibility.~~
