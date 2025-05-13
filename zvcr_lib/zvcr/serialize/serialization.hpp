#pragma once

#include <functional>
#include <fstream>
#include <zvcr/serialize/compression.hpp>
#include <zvcr/common/result.hpp>
#include <zvcr/common/data_storage.hpp>
#include <zvcr/region/dimension.hpp>
#include <zvcr/region/segment/segment_info.hpp>
#include <zvcr/region/dim3/segment3.hpp>
#include <zvcr/region/dim3/zvcr3.hpp>
#include <zvcr/region/dim2/segment2.hpp>
#include <zvcr/region/dim2/zvcr2.hpp>
#include <zvcr/region/dim2/layer.hpp>
#include <zvcr/region/file_location.hpp>
#include <zvcr/common/definitions.hpp>

namespace zvcr::serialize {

    using namespace result;
    using namespace reverse_delta;
    using namespace region;
    using namespace paletted_storage;
    using namespace definitions;

    using Palette = std::vector<SegmentAtom>;

    enum ZVCRError {
        FILE_NOT_FOUND,
        GENERIC_READ_ERROR,
        EXPECTED_DELTA_LENGTH,
        EXPECTED_TIMESTAMP,
        EXPECTED_PACKED_LENGTH,
        EXPECTED_PACKED_DATA,
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
        EXPECTED_TILE_ENTITIES_LENGTH,
        EXPECTED_TILE_ENTITY_COUNTS,
        EXPECTED_TILE_ENTITY_COUNTS_TIMESTAMP,
        EXPECTED_LAYER_TYPE,
        EXPECTED_LAYERS_LENGTH,
        MISSING_HEADER,
        INVALID_HEADER_PREFIX,
        INVALID_VERSION,
        INVALID_DIMENSION_TYPE
    };

    static constexpr auto ZSTD_COMPRESSION_LEVEL_DEFAULT = 12;
    static constexpr auto ZSTD_COMPRESSION_THREADS_DEFAULT = 4;

    namespace fs = std::filesystem;

    template<typename R>
    using ZVCRResult = Result<R, ZVCRError>;

    template<typename R>
    using ZVCRFileSerialize = std::function<void(const R&, std::vector<uint8_t>&)>;

    template<typename R>
    using ZVCRFileDeserialize = std::function<ZVCRResult<R>(const std::vector<uint8_t>&, size_t&, size_t)>;

    ZVCRResult<DimensionType> deserializeDimensionType(const std::vector<uint8_t>& data, size_t& offset);

    template<typename Version>
    ZVCRResult<Version> deserializeVersion(const std::vector<uint8_t>& data, size_t& offset, Version latest);

    Option<ZVCRError> validateZVCRFilePrefix(const std::vector<uint8_t>& data, size_t& offset, const std::string& prefix);

    void serializeZVCR2File(const ZVCR2File& file, std::vector<uint8_t>& data);

    ZVCRResult<ZVCR2File> deserializeZVCR2File(const std::vector<uint8_t>& data, size_t& offset, size_t maxDeltas);

    void serializeZVCR3File(const ZVCR3File& file, std::vector<uint8_t>& data);

    ZVCRResult<ZVCR3File> deserializeZVCR3File(const std::vector<uint8_t>& data, size_t& offset, size_t maxDeltas);

    template<typename R>
    struct DefaultSerialization {
        static_assert(std::is_same_v<R, ZVCR2File> || std::is_same_v<R, ZVCR3File>,
            "ZVCR serialization only supports dim2::zvcr2::ZVCR2File and dim3::zvcr3::ZVCR3File");
    };

    template<>
    struct DefaultSerialization<ZVCR2File> {
        static constexpr auto serialize = serializeZVCR2File;
        static constexpr auto deserialize = deserializeZVCR2File;
        static constexpr auto format = RegionFormat::ZVCR2;
    };

    template<>
    struct DefaultSerialization<ZVCR3File> {
        static constexpr auto serialize = serializeZVCR3File;
        static constexpr auto deserialize = deserializeZVCR3File;
        static constexpr auto format = RegionFormat::ZVCR3;
    };

    template<typename R>
    size_t writeZVCRFile(const R& file, const fs::path& filepath,
                         const int zstdCompressionLevel = ZSTD_COMPRESSION_LEVEL_DEFAULT,
                         const int zstdCompressionThreads = ZSTD_COMPRESSION_LEVEL_DEFAULT) {
        std::vector<uint8_t> bytesUncompressed;
        DefaultSerialization<R>::serialize(file, bytesUncompressed);
        const auto bytesCompressed = compressData(bytesUncompressed, zstdCompressionLevel, zstdCompressionThreads);

        std::ofstream fileStream(filepath, std::ios::out | std::ios::binary);
        fileStream.write(reinterpret_cast<const char*>(bytesCompressed.data()), static_cast<int64_t>(bytesCompressed.size()));
        fileStream.close();

        return bytesCompressed.size();
    }

    template<typename R>
    ZVCRResult<R> readZVCRFile(const fs::path& filepath, const size_t maxDeltas = 0) {
        if (!exists(filepath)) return Err(FILE_NOT_FOUND);

        try {
            std::ifstream fileStream(filepath, std::ios::in | std::ios::binary);

            fileStream.seekg(0, std::ios::end);
            const int64_t fileSize = fileStream.tellg();
            fileStream.seekg(0, std::ios::beg);

            const auto bytesCompressed = new char[fileSize];
            fileStream.read(bytesCompressed, fileSize);
            fileStream.close();

            const auto bytesCompressedVector = std::vector<uint8_t>(bytesCompressed, bytesCompressed + fileSize);
            const auto bytesUncompressed = decompressData(bytesCompressedVector);
            size_t offset{};

            delete[] bytesCompressed;
            return DefaultSerialization<R>::deserialize(bytesUncompressed, offset, maxDeltas);
        } catch (const std::length_error&) {
            return Err(GENERIC_READ_ERROR);
        }
    }

    template<typename R>
    size_t writeZVCRFileAt(const R& file, const fs::path& parentDirectory, const RegionLocation& location,
                           const int zstdCompressionLevel = ZSTD_COMPRESSION_LEVEL_DEFAULT,
                           const int zstdCompressionThreads = ZSTD_COMPRESSION_LEVEL_DEFAULT) {
        create_directories(location.getDirectory(parentDirectory));
        return writeZVCRFile<R>(file, location.getFilePath(parentDirectory, DefaultSerialization<R>::format),
                                zstdCompressionLevel, zstdCompressionThreads);
    }

    template<typename R>
    ZVCRResult<R> readZVCRFileAt(const fs::path& parentDirectory, const RegionLocation& location, const size_t maxDeltas = 0) {
        return readZVCRFile<R>(location.getFilePath(parentDirectory, DefaultSerialization<R>::format), maxDeltas);
    }

    void serializePackedSnapshot(const PackedSnapshot<SegmentAtom>& snapshot, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

    ZVCRResult<PackedSnapshot<SegmentAtom>> deserializePackedSnapshot(const std::vector<uint8_t>& data, size_t& offset,
                                                                      const std::vector<Palette>& paletteTable, size_t snapshotLength);

    void serializePaletteTable(const std::vector<Palette>& paletteTable, std::vector<uint8_t>& data);

    ZVCRResult<std::vector<Palette>> deserializePaletteTable(const std::vector<uint8_t>& data, size_t& offset);

    Option<ZVCRError> skipPackedSnapshot(const std::vector<uint8_t>& data, size_t& offset);

    void serializePackedDeltaData(const PackedDeltaData<SegmentAtom>& section3d, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

    ZVCRResult<PackedDeltaData<SegmentAtom>> deserializePackedDeltaData(const std::vector<uint8_t>& data, size_t& offset,
                                                                        const std::vector<Palette>& paletteTable, size_t maxDeltas, size_t snapshotLength);

    void serializeSegmentState(const SegmentState& segmentState, std::vector<uint8_t>& data);

    ZVCRResult<SegmentState> deserializeSegmentState(const std::vector<uint8_t>& data, size_t& offset);

    void serializeTileEntityCountInfo(const TileEntityCountInfo& tileEntityCounts, std::vector<uint8_t>& data);

    ZVCRResult<TileEntityCountInfo> deserializeTileEntityCountInfo(const std::vector<uint8_t>& data, size_t& offset);

    void serializeSegmentInfo(const SegmentInfo& segmentInfo, std::vector<uint8_t>& data);

    ZVCRResult<SegmentInfo> deserializeSegmentInfo(const std::vector<uint8_t>& data, size_t& offset);

    void serializeSegment3d(const Segment3d& segment3d, std::vector<uint8_t>& data,
                            std::vector<Palette>& paletteTable, ZVCR3Version version);

    ZVCRResult<Segment3d> deserializeSegment3d(const std::vector<uint8_t>& data, size_t& offset,
                                               const std::vector<Palette>& paletteTable, size_t maxDeltas,
                                               uint32_t sectionCount, ZVCR3Version version);

    void serializeOptSegment3d(const Option<Segment3d>& segment3dOpt, std::vector<uint8_t>& data,
                               std::vector<Palette>& paletteTable, ZVCR3Version version);

    ZVCRResult<Option<Segment3d>> deserializeOptSegment3d(const std::vector<uint8_t>& data, size_t& offset,
                                                          const std::vector<Palette>& paletteTable, size_t maxDeltas,
                                                          uint32_t sectionCount, ZVCR3Version version);

    void serializeRegion3d(const Region3d& region, std::vector<uint8_t>& data, ZVCR3Version version);

    ZVCRResult<Region3d> deserializeRegion3d(const std::vector<uint8_t>& data, size_t& offset, size_t maxDeltas,
                                             uint32_t sectionCount, ZVCR3Version version);

    void serializeLayer(const Layer2d& layer, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

    ZVCRResult<Layer2d> deserializeLayer(const std::vector<uint8_t>& data, size_t& offset,
                                         const std::vector<Palette>& paletteTable, size_t maxDeltas, size_t snapshotSize);

    void serializeLayers(const LayerContainer2d& layers, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

    ZVCRResult<LayerContainer2d> deserializeLayers(const std::vector<uint8_t>& data, size_t& offset,
                                                   const std::vector<Palette>& paletteTable, size_t maxDeltas, size_t snapshotSize);

    ZVCRResult<LayerContainer2d> deserializeBlockLayers(const std::vector<uint8_t>& data, size_t& offset,
                                                        const std::vector<Palette>& paletteTable, size_t maxDeltas);

    ZVCRResult<LayerContainer2d> deserializeBiomeLayers(const std::vector<uint8_t>& data, size_t& offset,
                                                        const std::vector<Palette>& paletteTable, size_t maxDeltas,
                                                        ZVCR2Version version);

    void serializeSegment2d(const Segment2d& segment, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable,
                            ZVCR2Version version);

    ZVCRResult<Segment2d> deserializeSegment2d(const std::vector<uint8_t>& data, size_t& offset,
                                               const std::vector<Palette>& paletteTable, size_t maxDeltas,
                                               ZVCR2Version version);

    void serializeOptSegment2d(const Option<Segment2d>& segment, std::vector<uint8_t>& data,
                               std::vector<Palette>& paletteTable, ZVCR2Version version);

    ZVCRResult<Option<Segment2d>> deserializeOptSegment2d(const std::vector<uint8_t>& data, size_t& offset,
                                                          const std::vector<Palette>& paletteTable, size_t maxDeltas,
                                                          ZVCR2Version version);

    void serializeRegion2d(const Region2d& region, std::vector<uint8_t>& data, ZVCR2Version version);

    ZVCRResult<Region2d> deserializeRegion2d(const std::vector<uint8_t>& data, size_t& offset, size_t maxDeltas, ZVCR2Version version);

}
