#pragma once

#include <zvcr/io/file_location.hpp>
#include <zvcr/io/file_type.hpp>
#include <zvcr/io/compression.hpp>
#include <zvcr/io/serialize/context.hpp>
#include <zvcr/region/paletted_delta_data.hpp>
#include <zvcr/region/tile_entities.hpp>
#include <fstream>
#include <zstd.h>
#include <fmt/core.h>

namespace zvcr {

    inline constexpr auto MAX_DELTA_LENGTH = 65536;
    inline constexpr auto MAX_SEGMENT_STATES_LENGTH = 65536;
    inline constexpr auto MAX_LEGACY_TILE_ENTITIES_LENGTH = 65536;
    inline constexpr auto MAX_TILE_ENTITY_LIST_LENGTH = 98304;
    inline constexpr auto MAX_TILE_ENTITY_NBT_LENGTH = 65536;
    inline constexpr auto MAX_PACKED_LENGTH = 1024;
    inline constexpr auto MAX_PALETTE_TABLE_LENGTH = 262144;

    enum ReadErrorType {
        FILE_NOT_FOUND,
        GENERIC_READ_ERROR,
        ZSTD_ERROR,
        EXPECTED_DELTA_LENGTH,
        EXPECTED_TIMESTAMP,
        EXPECTED_PACKED_LENGTH,
        EXPECTED_PACKED_DATA,
        EXPECTED_PALETTE_TYPE,
        EXPECTED_PALETTE_SINGLE_DATA,
        EXPECTED_PALETTE_INDEX,
        EXPECTED_PALETTE_TABLE_LENGTH,
        EXPECTED_PALETTE_LENGTH,
        EXPECTED_PALETTE_DATA,
        EXPECTED_SEGMENT_INDICATOR,
        EXPECTED_VERSION,
        EXPECTED_DIMENSION_TYPE,
        EXPECTED_SEGMENT_STATES_LENGTH,
        EXPECTED_SEGMENT_STATE_TYPE,
        EXPECTED_SEGMENT_STATE_TIMESTAMP,
        EXPECTED_PROTOCOL_VERSION,
        EXPECTED_LEGACY_TILE_ENTITIES_LENGTH,
        EXPECTED_LEGACY_TILE_ENTITY_COUNTS,
        EXPECTED_LEGACY_TILE_ENTITY_COUNTS_TIMESTAMP,
        EXPECTED_LAYER_TYPE,
        EXPECTED_LAYERS_LENGTH,
        EXPECTED_TILE_ENTITY_LIST_DELTAS_LENGTH,
        EXPECTED_TILE_ENTITY_LIST_TIMESTAMP,
        EXPECTED_TILE_ENTITY_LIST_LENGTH,
        EXPECTED_TILE_ENTITY_PACKED_POSITION,
        EXPECTED_TILE_ENTITY_DELTA_OPERATION,
        EXPECTED_TILE_ENTITY_TYPE,
        EXPECTED_TILE_ENTITY_NBT_LENGTH,
        EXPECTED_TILE_ENTITY_NBT,
        MISSING_HEADER,
        INVALID_HEADER_PREFIX,
        INVALID_VERSION,
        INVALID_DIMENSION_TYPE,
        INVALID_PALETTE_INDEX,
        INVALID_DELTA_LENGTH,
        INVALID_PACKED_LENGTH,
        INVALID_PALETTE_TABLE_LENGTH,
        INVALID_SEGMENT_STATES_LENGTH,
        INVALID_LEGACY_TILE_ENTITIES_LENGTH,
        INVALID_SEGMENT_STATE_ID,
        INVALID_TILE_ENTITY_LIST_DELTAS_LENGTH,
        INVALID_TILE_ENTITY_LIST_LENGTH,
        INVALID_TILE_ENTITY_NBT_LENGTH,
    };

    class ReadHandle;

    struct ReadError {

        static constexpr auto DBG_SLICE_SIZE = 32;

        ReadErrorType type;
        size_t offset;
        std::string message;
        std::vector<uint8_t> dumpSlice;
        size_t sliceStart;
        size_t sliceEnd;
        size_t dumpPoint;
        size_t dataLength;

        [[nodiscard]]
        auto what() const -> std::string;

        auto attach(const ReadHandle &handle) -> void;
    };

    template<typename T>
    using ReadResult = result::Result<T, ReadError>;

    class ReadHandle {
        size_t offset{};
        std::vector<Palette> blockPaletteTable{};
        std::vector<Palette> biomePaletteTable{};

    public:
        Context ctx{};
        std::vector<uint8_t> data;
        size_t maxDeltas;

        explicit ReadHandle(std::vector<uint8_t> data, const size_t maxDeltas):
            data(std::move(data)), maxDeltas(maxDeltas) {}

        [[nodiscard]]
        auto getOffset() const -> size_t {
            return offset;
        }

        [[nodiscard]]
        auto readByte(const ReadErrorType orElseErr) -> ReadResult<uint8_t>{
            if (offset >= data.size()) {
                const auto err = ReadError{orElseErr, offset, fmt::format("Read out of bounds: {} >= {}", offset, data.size())};
                return ERR(err);
            }
            return data[offset++];
        }

        template<typename T>
        [[nodiscard]]
        auto read(const ReadErrorType orElseErr) -> ReadResult<T> {
            if (offset + sizeof(T) > data.size()) {
                const auto err = ReadError{orElseErr, offset, fmt::format("Read out of bounds: {} + {} > {}", offset, sizeof(T), data.size())};
                return ERR(err);
            }
            T value;
            std::memcpy(&value, data.data() + offset, sizeof(T));
            offset += sizeof(T);
            return value;
        }

        template<typename T>
        [[nodiscard]]
        auto readArray(std::vector<T> &array, const ReadErrorType orElseErr) -> ReadResult<std::monostate>{
            const auto length = array.size();
            if (offset + length * sizeof(T) > data.size())
                return ERR(ReadError(orElseErr, offset, fmt::format("Read out of bounds: {} + {} > {}", offset, length * sizeof(T), data.size())));

            std::memcpy(array.data(), data.data() + offset, length * sizeof(T));
            offset += length * sizeof(T);
            return {};
        }

        template<typename T>
        [[nodiscard]]
        auto readArray(T *array, const size_t length, const ReadErrorType orElseErr) -> ReadResult<std::monostate> {
            if (offset + length * sizeof(T) > data.size())
                return ERR(ReadError(orElseErr, offset, fmt::format("Read out of bounds: {} + {} > {}", offset, length * sizeof(T), data.size())));

            std::memcpy(array, data.data() + offset, length * sizeof(T));
            offset += length * sizeof(T);
            return {};
        }

        template<typename T>
        [[nodiscard]]
        auto skip(const size_t n, const ReadErrorType orElseErr) -> ReadResult<std::monostate> {
            const auto length = n * sizeof(T);
            if (offset + length > data.size())
                return ERR(ReadError(orElseErr, offset, fmt::format("Read out of bounds: {} + {} > {}", offset, length, data.size())));

            offset += length;
            return {};
        }

        template<typename T>
        [[nodiscard]]
        auto skip(const ReadErrorType orElseErr) -> ReadResult<std::monostate> {
            return skip<T>(1, orElseErr);
        }

        [[nodiscard]]
        auto deserializeVersion(const Version latest) -> ReadResult<Version> {
            const auto versionNumber = TRY(readByte(EXPECTED_VERSION));

            if (versionNumber > static_cast<uint8_t>(latest)) {
                const auto err = ReadError{INVALID_VERSION, offset,
                    fmt::format("Invalid zvcr version number: {}; Maximum allowed version number: {}", versionNumber, static_cast<int>(latest))};
                return ERR(err);
            }
            return static_cast<Version>(versionNumber);
        }

        [[nodiscard]]
        auto validateFilePrefix(std::string_view prefix) -> ReadResult<std::monostate>;

        [[nodiscard]]
        auto deserializeDimensionType() -> ReadResult<DimensionType>;

        template<size_t unpackedSize>
        [[nodiscard]]
        auto deserializePackedSnapshot(PackedSnapshot<unpackedSize> &snapshot, const std::vector<Palette> &paletteTable) -> ReadResult<std::monostate> {
            snapshot.timestamp = static_cast<time_t>(TRY(read<uint64_t>(EXPECTED_TIMESTAMP)));

            if (ctx.supportSingleValuePalette) {
                const auto dataType = TRY(read<uint8_t>(EXPECTED_PALETTE_TYPE));
                if (dataType == 0) {
                    const auto singleValue = TRY(read<uint16_t>(EXPECTED_PALETTE_SINGLE_DATA));
                    snapshot.data.data = singleValue;
                    return {};
                }
            }
            const auto packedLength = TRY(read<uint64_t>(EXPECTED_PACKED_LENGTH));
            if (packedLength > MAX_PACKED_LENGTH) {
                const auto err = ReadError{INVALID_PACKED_LENGTH, offset, fmt::format("Invalid packed length: {} > {}", packedLength, MAX_PACKED_LENGTH)};
                return ERR(err);
            }
            auto palettedData = PalettedData<unpackedSize>{};

            auto &packedLongArray = palettedData.packedLongArray;
            if (packedLength > packedLongArray.size())
                packedLongArray.resize(packedLength);

            TRY(readArray(packedLongArray.data(), packedLength, EXPECTED_PACKED_DATA));
            palettedData.bitStorageLegacy.data = packedLongArray;
            palettedData.bitStorageLegacy.size = unpackedSize;

            const auto paletteIndex = TRY(read<uint32_t>(EXPECTED_PALETTE_INDEX));
            if (paletteIndex == UINT32_MAX) {
                // direct palette update; use uint32 max to encode direct palette
                palettedData.palette = DIRECT_PALETTE;
                palettedData.bitStorageLegacy.bits = palettedData.palette.bitsPerEntry;
                palettedData.bitStorageLegacy.init();
                snapshot.data.data = palettedData;
                return {};
            }
            if (paletteIndex >= paletteTable.size()) {
                const auto err = ReadError{INVALID_PALETTE_INDEX, offset, fmt::format("Invalid palette index: {} >= {}", paletteIndex, paletteTable.size())};
                return ERR(err);
            }
            const auto &palette = paletteTable.at(paletteIndex);
            if (palette.length() == 1) {
                snapshot.data.data = palette.palette[0]; // single value palette update; backwards compatibility
                return {};
            }
            palettedData.palette = palette;
            palettedData.bitStorageLegacy.bits = palettedData.palette.bitsPerEntry;
            palettedData.bitStorageLegacy.init();
            snapshot.data.data = palettedData;
            return {};
        }

        [[nodiscard]]
        auto deserializePaletteTable(std::vector<Palette> &paletteTable) -> ReadResult<std::monostate>;

        [[nodiscard]]
        auto skipPackedSnapshot() -> ReadResult<std::monostate>;

        template<size_t unpackedSize>
        [[nodiscard]]
        auto deserializePackedDeltaData(PackedDeltaData<unpackedSize> &reverseDeltas, const std::vector<Palette> &paletteTable) -> ReadResult<std::monostate> {
            const auto deltaLength = TRY(read<uint64_t>(EXPECTED_DELTA_LENGTH));
            if (deltaLength > MAX_DELTA_LENGTH) {
                const auto err = ReadError{INVALID_DELTA_LENGTH, offset, fmt::format("Invalid delta length: {} > {}", deltaLength, MAX_DELTA_LENGTH)};
                return ERR(err);
            }
            reverseDeltas.reverseDeltas.resize(deltaLength);
            for (size_t deltaIndex = 0; deltaIndex < deltaLength; ++deltaIndex) {
                if (maxDeltas != 0 && deltaIndex >= maxDeltas) {
                    TRY(skipPackedSnapshot());
                    continue;
                }
                TRY(deserializePackedSnapshot<unpackedSize>(reverseDeltas.reverseDeltas[deltaIndex], paletteTable));
            }
            return {};
        }

        [[nodiscard]]
        auto deserializeSegmentState() -> ReadResult<SegmentState>;

        [[nodiscard]]
        auto deserializeTileEntityCountInfo() -> ReadResult<std::monostate>;

        [[nodiscard]]
        auto deserializeSegmentInfo() -> ReadResult<SegmentInfo>;

        [[nodiscard]]
        auto deserializeTileEntities() -> ReadResult<DeltaTileEntityData>;

        [[nodiscard]]
        auto deserializeSegment() -> ReadResult<std::shared_ptr<Segment>>;

        [[nodiscard]]
        auto deserializeRegion(Region &region) -> ReadResult<std::monostate>;
    };

    [[deprecated]]
    auto deserializeFileLegacy(ReadHandle &handle) -> ReadResult<File>;

    auto deserializeFile(ReadHandle &handle) -> ReadResult<File>;

    [[deprecated]]
    auto readFileLegacy(const fs::path &filepath, size_t maxDeltas = 0) -> ReadResult<File>;

    auto readFile(const fs::path &filepath, size_t maxDeltas = 0) -> ReadResult<File>;

    [[deprecated]]
    auto readFileAtLegacy(const fs::path &parentDirectory, const RegionLocation &location,
                          size_t maxDeltas = 0) -> ReadResult<File>;

    auto readFileAt(const fs::path &parentDirectory, const RegionLocation &location,
                    size_t maxDeltas = 0) -> ReadResult<File>;
}
