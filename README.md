# Zstd-compressed Version Controlled Region (ZVCR)
A C++ library for handling the ZVCR-3D file format, enabling a more feature-rich and efficient storage of Minecraft worlds.

The intended use case of ZVCR is limited to Minecraft Java Edition world data archival, rather than replacing the
Minecraft Anvil format (mca) in general. The ZVCR file formats officially support Minecraft versions 1.20.4 and above.

# Compression
ZVCR files use [Zstd](https://github.com/facebook/zstd) compression alongside 
[packed data using palettes](#packing-and-unpacking) to minimize the compression ratio. For historical data,
[deltas](#packed-delta-data) are used, such that older snapshots can be easily reconstructed, instead of duplicating 
large amounts of almost equal data.

ZVCR file headers are left uncompressed. The [Region Container](#region-container) is compressed with a Zstd level of 8
by default (compared to the Anvil region format traditionally compressing individual chunks). The decision to compress 
the entire region, instead of individual chunks, was made specifically to benefit the compression ratio. Higher
levels of Zstd can reduce the filesize by small amounts, although they come with a great performance penalty. Zstd level
8 was chosen as a default as it was the perfect compression ratio to compression speed tradeoff.

The ZVCR file format was never intended for use in a full Minecraft server, where actively reading and writing individual
chunks on demand is common, and was instead created purely for long term data archival. The side effects of this 
(in memory usage) when used in a world downloader server, or in a 
[PlaceViewer](https://github.com/2b2tplace/PlaceViewer) server, are minimal.

Testing of region-level Zstd compression has also proven successful in the 
[Linear region format](https://github.com/xymb-endcrystalme/LinearRegionFileFormatTools),
seeing similar benefits, even though that file format was created for use in running actual Minecraft servers.

# Endianness
Most of ZVCR is explicitly stored in little-endian. The only exception is NBT data found in tile entities, which is
serialized in big-endian. This decision was made purely for a standard NBT implementation that can easily interoperate
with Minecraft.

# Include it in your project
Add the following to your CMakeLists.txt:
```cmake
include(FetchContent)

FetchContent_Declare(zvcr
        GIT_REPOSITORY https://github.com/2b2tplace/zvcr.git
        GIT_TAG main
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(zvcr)
target_link_libraries(my_project
        PRIVATE zvcr_lib
)
```

# ZVCR Directory Structure Specification
Files in the ZVCR format can technically be stored in any arbitrary directory, and can technically have any file name. 
When storing a large amount of ZVCR files for a Minecraft world however, following a standardized directory structure and
having standardized positional file names is very much required. All official ZVCR-related tooling expects files to be 
placed in a directory that looks something like this:

- Each ZVCR region file is located in `parentDirectory/{dimension}/{sectorX}/{sectorZ}/r.{regionX}.{regionZ}.zvcr3d`.
- There are three standard dimension directory names corresponding to the dimensions found in vanilla Minecraft: 
`overworld`, `nether`, and `end`. Modded dimensions are currently not supported by the ZVCR file format. A ZVCR 
directory may not necesarily contain all vanilla dimensions.
- Sector coordinates `sector{X|Z}` are calculated using `floor(region{X|Z} / 32)` (commonly denoted as 
`floorDiv(region{X|Z}, 32)`, usually calculated with `region{X|Z} >> 5` (\*not always, see below)).
- ZVCR Region coordinates are equivalent to 
[Minecraft Anvil region coordinates](https://minecraft.tools/en/coordinate-calculator.php). A region located at
`(regionX, regionZ)` contains all blocks in an area between `(regionX * 512, regionZ * 512)` and
`((regionX + 1) * 512 - 1, (regionZ + 1) * 512 - 1)`, corresponding to absolute block coordinates X and Z in the given
dimension.

For example, ZVCR regions `0.0`, `0.-1`, `-1.0`, `-1.-1` in the overworld dimension, corresponding to a square area 
defined by the corner block coordinates `(-512, -512)` and `(511, 511)`:
```
parentDirectory
└── overworld
    ├── 0
    │   ├── 0
    │   │   └── r.0.0.zvcr3d
    │   └── -1
    │       └── r.0.-1.zvcr3d
    └── -1
        ├── 0
        │   └── r.-1.0.zvcr3d
        └── -1
            └── r.-1.-1.zvcr3d
```

\
\
(\*) The signed bitshift operator `>>` behavior may be implementation-defined in C and C++. In particular, right-shifting
a negative signed integer may perform either an arithmetic shift (sign-extending) or a logical shift, depending on the 
implementation.

When portability is important, sector division should be performed using a floor division operation equivalent to 
`floorDiv(regionCoordinate, 32)`, or the following function, computing `floor(regionCoordinate, 32)` for all 
signed integers:
```cpp
int32_t floorDiv32(const int32_t regionCoordinate) {
    if (regionCoordinate >= 0) {
        return regionCoordinate / 32;
    } else {
        return (regionCoordinate - 31) / 32;
    }
}
```
As such, each sector contains a maximum of 32 * 32 = 1024 regions. It is very important to use `floorDiv32`, 
an arithmetic 5-right-bitshift on signed integers, or an equivalent mathematical expression when converting region 
coordinates to sector coordinates.

Simply using the integer division operator `/` in most languages yields a result truncating toward zero, which is not 
the expected behavior for this calculation. For example, `-1 / 32 = -0.03125` becomes `0` in integer division, truncated
toward zero. The expected value, however, is `floorDiv(-1, 32) = floor(-1.0 / -32.0) = -1`.

# ZVCR File Format Specification
| Field                 | Type                                                                                                                              |
|-----------------------|-----------------------------------------------------------------------------------------------------------------------------------|
| ZVCR Magic Prefix     | fixed-size `string` without a length prefix; Always be equal to `zvcr3d`.                                                         |
| ZVCR Version Number   | `uint8`, see [Version Numbers](#zvcr-versions-and-version-numbers)                                                                |
| Dimension Type        | `uint8`, see [Dimension Type Encoding](#dimension-type-encoding)                                                                  |
| Protocol Version (\*) | `uint16`, see [Protocol Version Numbers](https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge/Protocol_version_numbers) |
| Region Container      | [Region Container](#region-container), Zstd-compressed at level 8 by default                                                      |

### Registries
(\*) The Protocol Version is used to determine which registries of the game are used. This importantly dictates which block
state IDs (note: not Block IDs, but specifically numeric Blockstate IDs), Biome IDs and tile entity IDs/NBT formats
to use. Whenever the ZVCR documentation states Blockstate ID, Biome ID, or tile entity ID, encoded as an unsigned
integer, that numeric ID refers to the particular entry with that ID in the registry of the given context. Registries can
be generated using [data generators](https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge/Data_Generators),
extracting information from Minecraft server/client JAR files. These numeric IDs usually are not found explicitly within
Minecraft source code, and are rather implied from the order the entries were defined in. Furthermore, Biome IDs
only refer to default biomes as provided on a vanilla Java Edition Minecraft server. Any additional content added via
data packs or mods is currently unsupported by zvcr. 

Example registries for Minecraft version 1.21.4 (Protocol version number 769) can be found
[here](https://github.com/2b2tplace/mc-cpp/tree/main/registries/769).

The version being specified at the region-level instead of within individual chunks prevents version mismatches within 
a single ZVCR world save. This implies partially upgrading some chunks but not others in ZVCR is not possible, and instead 
the entire world must be upgraded to a newer Minecraft version. A one-time upgrading operation is more beneficial for 
consistency and performance, compared to upgrading on an as-needed basis, especially when handling ZVCR files in a 
read-only context.

## ZVCR Versions and Version Numbers
ZVCR versions follow an extended form of Semantic Versioning ([SemVer](https://github.com/semver/semver/blob/master/semver.md)), 
represented as `RELEASE.MAJOR.MINOR.PATCH`. The `RELEASE` component is incremented only in rare cases involving
fundamental changes to the format that affect the entire specification.

The Git tags for this sample implementation repository follow the same versioning scheme. These versions apply to the 
implementation itself and should not be interpreted as versions of the ZVCR file format.

The `RELEASE` number was increased to `1` upon finalization of the first public release of ZVCR. Versions preceding
`1.X.X.X` were experimental and used exclusively for internal development, and they are no longer supported.

Support for older versions is limited to be read-only. Any files written using this implementation will always use 
the latest ZVCR version.

### Version Number Encoding
| Version number | Version | Changes                                         | Supported |
|----------------|---------|-------------------------------------------------|-----------|
| 7              | 1.0.0.0 | See [Release 1 Changelog](#release-1-changelog) | Yes       |
| 6              | 0.1.4.0 |                                                 | No        |
| 5              | 0.1.3.0 |                                                 | No        |
| 4              | 0.1.2.0 |                                                 | No        |
| 3              | 0.1.1.0 |                                                 | No        |
| 2              | 0.1.0.0 |                                                 | No        |
| 1              | 0.0.0.1 |                                                 | No        |
| 0              | 0.0.0.0 |                                                 | No        |

### Release 1 Changelog
- The ZVCR file header is no longer compressed. ZVCR Magic Prefix, ZVCR Version Number, Dimension Type and Protocol 
Version are written directly. The Palette Table + all Segments are compressed into one Zstd buffer.
- The ZVCR Magic Prefix in the ZVCR header was changed from ZVRegion to zvcr3d.
- Bits per entry is now rounded up to the nearest multiple of 4. Use 4 bits if the palette has 1..=16 unique values, 
8 bits if it has 17..=256 unique values. For palettes requiring more than 8 bits to represent, bits per entry is still
rounded up to 16 bits, switching to direct palette mode as before.
- There are now 2 distinct Palette Tables for Blocks and Biomes, instead of being one combined Palette Table.
- The default Zstd level was changed from 10 to 8. Levels above 8 are substantially slower for write operations, with
negligible gains to compression ratio. This is especially true for the changed bits per entry rounding.
- All ZVCR-2D file format support has been dropped.

Changes have also been made to the ZVCR file extensions and directory structure:
- The File extension was previously `.zvcr3`, now it has been renamed to `.zvcr3d`.
- The old directory structure code used integer division by 32 to calculate sector coordinates (`regionCoordinate / 32`).
This has been updated to the intended logic of using `floorDiv32(regionCoordinate)` instead.

### Dimension Type Encoding
The world height, and consequently the number of chunk sections, depends on the dimension type. This is currently 
hard-coded for each of the vanilla Minecraft dimension types, but support for modded dimensions is planned. 

### Unsigned Y Levels
Importantly, Y levels defined all throughout ZVCR are unsigned, with Y = 0 always being the bottom of the world. 
Converting Y levels between the base game and ZVCR is trivial (zvcr_y + min_y = minecraft_y). This was an intentional
design choice to make handling ZVCR files alone a little easier. The potential for confusion only appears in the
event when writing code that handles converting ZVCR and Minecraft Y levels.

| Dimension type | Number | Section count | Minimum Block Y (Minecraft) | World Block Height\* |
|----------------|--------|---------------|-----------------------------|----------------------|
| Overworld      | 0      | 24            | -64                         | 384                  |
| Nether         | 1      | 16            | 0                           | 256                  |
| The End        | 2      | 16            | 0                           | 256                  |

(\*) World Block Height here refers to the actual height of the world in blocks (section count \* 16), and not the maximum 
block Y level in Minecraft (which is 320 = 384 + (-64) in the Overworld for instance).

### Region container
A ZVCR region consists out of 32 * 32 = 1024 Segments, or "Chunks" in common terminology, similar to Minecraft Anvil
regions. For a given segment with local coordinates `segment{X|Z}` in the range of `0..32`, the index of this segment in
the segment array can be calculated using the following:
```cpp
size_t segmentIndex(const uint8_t x, const uint8_t z) {
    assert(x < 32 && z < 32);
    return static_cast<size_t>(x) * 32 + static_cast<size_t>(z);
}
```

| Field                  | Type                                                              |
|------------------------|-------------------------------------------------------------------|
| Block Palette Table    | [Indirect Palette Table](#indirect-palette-table)                 |
| Biome Palette Table    | [Indirect Palette Table](#indirect-palette-table)                 |
| 1024 Optional Segments | `array` of [Optional Segment](#optional-segment) with length 1024 |

## Paletted storage
### Packing and Unpacking
Data is packed per section snapshot using palettes, similar to the Minecraft Anvil file format. There are still some key
differences.

Unpacked data is stored as a `uint16 array` of size `unpacked size`. Entries within an unpacked data buffer are referred
to as "atoms" in this sample implementation, as these are used to represent various building blocks depending on the
context (Blockstate IDs or Biome IDs).

Packed data is stored as packed `uint64 array`, where each `uint64` can be divided into several entries, each referring
either to an index within the indirect palette associated with the packed data, or a direct value, corresponding to an
atom directly.

The number of bits per entry is calculated as such:
```cpp
size_t bitsPerEntry(size_t paletteLength) {
    if (paletteLength <= 16) return 4;
    if (paletteLength <= 256) return 8;

    return 16;
}
```
Intuitively, more storage could be saved by choosing bits per entry = `max(1, ceil(log2(paletteLength)))` for 
`paletteLength > 1`, and as such, packing many more values per `uint64`. Instead, ZVCR rounds up the bits per entry to
the nearest multiple of 4, as Zstd tends to perform better on byte-aligned data, and counter-intuitively, this actually
yields a better compression ratio (roughly a 14% reduction on real-world Minecraft terrain data). This also substantially
simplifies the core logic for packing/unpacking in general.

The number of entries per `uint64`, as well as the packed `uint64 array` length is calculated as such:
```cpp
size_t valuesPerLong = 64 / bitsPerEntry;
size_t packedArrayLength = (unpackedSize + valuesPerLong - 1) / valuesPerLong;
```

### Indirect Palette
A Palette may contain Blockstate IDs or Biome IDs depending on the context. Palette entries are stored as `uint16`.
For IDs referring to in-game content, [game registries](#registries) are used to refer to specific Blockstates or Biomes.

| Field            | Type                         |
|------------------|------------------------------|
| Palette Length n | `uint16`                     |
| Palette Data     | `uint16 array` with length n |

### Direct Palette
If the bits per entry of a palette exceeds 8, storing the packed data + the palette results in actually wasting storage.
To combat this, direct mode is used for any palette with bits per entry > 8.
Direct mode uses a 1:1 mapping of `uint16_t` entries to values rather than palette indices, and avoids palette usage
altogether. The entries are still packed in a `uint64 array`, and the bits per entry is hard coded to 16.

### Packed Delta Data
| Field              | Type                                                         |
|--------------------|--------------------------------------------------------------|
| Delta Length n     | `uint64`                                                     |
| n Packed Snapshots | `array` of [Packed Snapshot](#packed-snapshot) with length n |

### Packed Snapshot
A snapshot represents a palette packed delta data snapshot with a given fixed `unpacked size` when unpacked.
The latest snapshot (the first one read in delta data) contains all data in this snapshot. All snapshots after it are 
represented with reverse deltas, which allows for the full recreation of older data by applying deltas on top of the
latest snapshot.

To reconstruct a snapshot before a specific timestamp, the steps are simple:
- Start at the latest snapshot (always located at index = 0 in the list of `reverse deltas`). Store this in a temporary
unpacked data buffer (`uint16 array`) of size `unpacked size`.
- Iterate through `reverse deltas` until the desired timestamp has been reached or exceeded.
- For each `reverse delta` snapshot, unpack the packed snapshot. For each atom not equal to `0xFFFF` (representing an 
unchanged atom) in this unpacked buffer, overwrite the atom at its index in the temporary buffer with the atom found in
this particular unpacked buffer.
- The temporary buffer after all required iterations should be precisely equal to the snapshot before a given timestamp.

See [the implementation of paletted data storage](/zvcr_lib/src/zvcr/region/paletted_delta_data.hpp)
for a sample implementation of packing/unpacking, as well as the reverse delta algorithm.

| Field           | Type                                                                                                                             |
|-----------------|----------------------------------------------------------------------------------------------------------------------------------|
| Timestamp       | `uint64` (Unix time, seconds)                                                                                                    |
| Palette Type    | `uint8`, = 0 indicates a single-value palette, = 1 indicates section palette                                                     |
| Palette Value   | `uint16`, only present if single-value palette was used                                                                          |  
| Packed Length n | `uint64`, only present if section palette was used                                                                               |
| Packed Data     | `uint64 array` of length n, only present if section palette was used                                                             |
| Palette Index   | `uint32`, only present if section palette was used, index in the given palette table, `UINT32_MAX` to encode direct palette mode |

### Indirect Palette Table
A Palette Table contains an array of all unique Indirect Palettes used in this ZVCR file. Palettes with equal entries
but different entry order are treated as two unique palettes.

| Field                           | Type                                                           |
|---------------------------------|----------------------------------------------------------------|
| Indirect Palette Table Length n | `uint32`                                                       |
| n Indirect Palettes             | `array` of [Indirect Palette](#indirect-palette) with length n |

Direct and single-value palettes are not stored in this table.

## Segments
Segments describe a Minecraft chunk embedded within the region (16 blocks or 4 biomes in sidelength). A Segment contains
different data depending on which format it was stored in.

## Optional Segment
| Field               | Type                | Note                                                 |
|---------------------|---------------------|------------------------------------------------------|
| Segment Indicator   | `uint8`             | Boolean, zero indicating the absence of this segment |
| Segment             | [Segment](#segment) | Only present if the segment indicator was nonzero    |

## Segment
A Segment extends vertically and consists of n block and biome sections
(n depending on the [dimension type](#dimension-type-encoding)). The sections are ordered by the section Y level in
ascending order, such that section index = `0` corresponds to the lowest section Y in the given dimension
(e.g. section Y = -4 in overworld), and section index = `n - 1` corresponds to the highest section Y in the given
dimension (e.g. section Y = 19 in overworld).

| Field               | Type                                                                           |
|---------------------|--------------------------------------------------------------------------------|
| n Block Sections    | [Packed Delta Data](#packed-delta-data) with `unpacked size` = 16x16x16 = 4096 |
| n Biome Sections    | [Packed Delta Data](#packed-delta-data) with `unpacked size` = 4x4x4 = 64      |
| Segment Info        | [Segment Info](#segment-info)                                                  |
| Tile Entity History | [Tile Entity History](#tile-entity-history)                                    |

### Segment Info
Additional segment info is stored for miscellaneous applications. This currently includes seeing whether chunks on a 
Minecraft server have been newly generated or if they were loaded from disk.

| Field                         | Type                                                     |
|-------------------------------|----------------------------------------------------------|
| Segment States Length n       | `uint64`                                                 |
| n Segment States              | `array` of [Segment State](#segment-state) with length n |

### Segment State
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
| Field               | Type                      | Note                                              |
|---------------------|---------------------------|---------------------------------------------------|
| Packed Position     | `uint32`                  | Calculated as below (\*)                          |
| Operation           | `uint8`                   | 1 indicating "put", 0 indicating "erase" (\*\*)   |
| Tile Entity Type ID | `uint32`                  | Only present if Operation was nonzero (\*\*\*)    |
| NBT Data Length n   | `uint64`                  | Only present if Operation was nonzero             |
| NBT Data Buffer     | `uint8 array` of length n | Only present if Operation was nonzero. (\*\*\*\*) |

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

(\*\*\*) The Tile Entity Type ID refers to the specific tile entity in the selected protocol version, and can be found
in the [game registries](#registries).

(\*\*\*\*) NBT data is stored as a raw byte buffer following the
[NBT specification, as seen on the Minecraft wiki](https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge/NBT#Specification).
Specifically, the NBT dialect from the Java Edition of Minecraft (1.20.4+) is used, where data is stored in big endian.
An important distinction here is the use of [modified UTF-8](https://docs.oracle.com/javase/8/docs/api/java/io/DataInput.html#modified-utf-8)
for strings, as simply writing out strings in regular UTF-8 is not supported by the Java Edition of Minecraft. Using
modified UTF-8 is especially necessary when encountering Unicode codepoints in the range U+10000 to U+10FFFF which are 
traditionally stored in 4 bytes in UTF-8, but are stored using 6 bytes in modified UTF-8. Disregarding this requirement
will result in Java Edition clients disconnecting from the server, when receiving NBT data with strings wrongly encoded 
in UTF-8 instead of modified UTF-8. See below for the code snippet where this would become an obvious issue.

The NBT format used for tile entities in particular follows the same structure as when sent over the network. To be more
specific, tile entity NBT in ZVCR does not contain the `id`, `keepPacked`, `x`, `y`, and `z` NBT tags that are found in
tile entity NBT used in Anvil region files. Instead of NBT tags, the `Tile entity type` (replacing `id`) and `Packed position`
(replacing `x`, `y`, `z`) fields are used here. `keepPacked` is a special NBT tag used in the base game to differentiate 
invalid tile entities in Anvil region files, and is not present in ZVCR. Serialized NBT in ZVCR files can be directly used
during the construction of packets, without the need to (de)serialize an of the NBT data. This is a consequence of storing
tile entities as-is when received from a Java Edition Minecraft server.

Example from PlaceViewer, where tile entities are read directly from a ZVCR region file and then sent in a Chunk Data 
and Update Light packet (`buffer` in this code snippet is the packet buffer, which is being written into):
```cpp
const auto &tileEntities = segment->tileEntities.snapshotFrom(timestamp).value_or(zvcr::TileEntityList{});
pc::WriteData<pc::VarInt>(static_cast<int32_t>(tileEntities.size()), buffer);
for (const auto &[pos, tileEntity] : tileEntities) {
    pc::WriteData<uint8_t>(static_cast<uint8_t>((pos.x & 15) << 4 | pos.z & 15), buffer);
    pc::WriteData<int16_t>(static_cast<int16_t>(pos.y + minY), buffer);
    pc::WriteData<pc::VarInt>(static_cast<int32_t>(tileEntity.type), buffer);
    
    // Directly insert the NBT data from ZVCR into the Data field of the tile entity.
    buffer.insert(buffer.end(), tileEntity.nbt.begin(), tileEntity.nbt.end());
}
```

In ZVCR, NBT tags are always sorted alphabetically by their keys when found in NBT tag compounds. This alphabetical 
sorting should occur before serializing NBT to a byte buffer, when passed into a 
[zvcr::TileEntity](zvcr_lib/src/zvcr/region/tile_entities.hpp) structure. Sorting NBT keys this way is currently 
a requirement, as this ZVCR implementation does not include NBT (de)serialization, and adding NBT comparisons that ignore
key order would be more expensive, compared to directly checking NBT byte buffers for equality. Disregarding this 
requirement will result in tile entity deltas being wrongly created, due to serialized NBT byte buffers being different,
despite the underlying NBT data still being the same. This is not necessarily a big issue, but it will inflate the 
filesize. This is a temporary fix in the current implementation of ZVCR, and will likely be fixed by including an NBT 
library and doing proper NBT compound comparisons. 

The NBT data buffer also remains uncompressed (may change in the future, signaling this by using a different Operation number).