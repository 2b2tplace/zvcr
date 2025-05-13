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

    enum ReadError {
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
        EXPECTED_PROTOCOL_VERSION,
        EXPECTED_TILE_ENTITIES_LENGTH,
        EXPECTED_TILE_ENTITY_COUNTS,
        EXPECTED_TILE_ENTITY_COUNTS_TIMESTAMP,
        EXPECTED_LAYER_TYPE,
        EXPECTED_LAYERS_LENGTH,
        MISSING_HEADER,
        INVALID_HEADER_PREFIX,
        INVALID_VERSION,
        INVALID_DIMENSION_TYPE,
        INVALID_PALETTE_INDEX
    };

    static constexpr auto ZSTD_COMPRESSION_LEVEL_DEFAULT = 12;
    static constexpr auto ZSTD_COMPRESSION_THREADS_DEFAULT = 4;

    namespace fs = std::filesystem;

    template<typename R>
    using ReadResult = Result<R, ReadError>;

    template<typename R>
    using FileSerialize = std::function<void(const R&, std::vector<uint8_t>&)>;

    template<typename R>
    using FileDeserialize = std::function<ReadResult<R>(const std::vector<uint8_t>&, size_t&, size_t)>;

    static constexpr uint16_t PROTOCOL_VERSION_ZVCR_0_0_0_X = 765; // 1.20.4

    struct Context {
        bool supportBiomes{};
        bool supportDynamicVersioning{};
        uint16_t protocolVersion{};

        void initialize(const ZVCR3Version version) {
            supportBiomes = version >= ZVCR3Version::ZVCR3_0_1_0_0;
            supportDynamicVersioning = version >= ZVCR3Version::ZVCR3_0_1_1_0;
            protocolVersion = version == ZVCR3Version::ZVCR3_0_0_0_1
                                ? PROTOCOL_VERSION_ZVCR_0_0_0_X
                                : PROTOCOL_VERSION;
        }

        void initialize(const ZVCR2Version version) {
            supportBiomes = version >= ZVCR2Version::ZVCR2_0_1_0_0;
            supportDynamicVersioning = version >= ZVCR2Version::ZVCR2_0_1_1_0;
            protocolVersion = version == ZVCR2Version::ZVCR2_0_0_0_0
                                ? PROTOCOL_VERSION_ZVCR_0_0_0_X
                                : PROTOCOL_VERSION;
        }
    };

    class WriteHandle {
        Context ctx;
        std::vector<Palette> paletteTable;

    public:
        std::vector<uint8_t> data;

        template<typename T>
        void write(const T& value) {
            data.resize(data.size() + sizeof(T));
            std::memcpy(data.data() + data.size() - sizeof(T), &value, sizeof(T));
        }

        template<typename T>
        void writeArray(const T* array, const size_t length) {
            data.resize(data.size() + length * sizeof(T));
            std::memcpy(data.data() + data.size() - length, array, length);
        }

        template<typename Source>
        void writeBytes(const Source& source) {
            data.insert(data.end(), source.begin(), source.end());
        }

        void writeByte(const uint8_t byte) {
            data.push_back(byte);
        }

        void serializePackedSnapshot(const PackedSnapshot<SegmentAtom>& snapshot);

        void serializePaletteTable(const std::vector<Palette>& paletteTable);

        void serializePackedDeltaData(const PackedDeltaData<SegmentAtom>& section3d);

        void serializeSegmentState(const SegmentState& segmentState);

        void serializeTileEntityCountInfo(const TileEntityCountInfo& tileEntityCounts);

        void serializeSegmentInfo(const SegmentInfo& segmentInfo);

        void serializeSegment3d(const Segment3d& segment3d);

        void serializeOptSegment3d(const Option<Segment3d>& segment3dOpt);

        void serializeRegion3d(const Region3d& region, ZVCR3Version version);

        void serializeLayer(const Layer2d& layer);

        void serializeLayers(const LayerContainer2d& layers);

        void serializeSegment2d(const Segment2d& segment);

        void serializeOptSegment2d(const Option<Segment2d>& segment);

        void serializeRegion2d(const Region2d& region, ZVCR2Version version);
    };

    class ReadHandle {
        Context ctx{};
        size_t offset{};
        std::vector<Palette> paletteTable{};
        uint32_t sectionCount{};

        size_t maxDeltas;

    public:
        const std::vector<uint8_t>& data;

        explicit ReadHandle(const std::vector<uint8_t>& data, const size_t maxDeltas):
            maxDeltas(maxDeltas), data(data) {}

        ReadResult<uint8_t> readByte(const ReadError orElseErr) {
            if (offset >= data.size())
                return Err(orElseErr);

            return data[offset++];
        }

        template<typename T>
        ReadResult<T> read(const ReadError orElseErr) {
            if (offset + sizeof(T) > data.size())
                return Err(orElseErr);

            T value;
            std::memcpy(&value, data.data() + offset, sizeof(T));
            offset += sizeof(T);
            return value;
        }

        template<typename T>
        Option<ReadError> readArray(std::vector<T>& array, const ReadError orElseErr) {
            const auto length = array.size();
            if (offset + length * sizeof(T) > data.size())
                return orElseErr;

            std::memcpy(array.data(), data.data() + offset, length * sizeof(T));
            offset += length * sizeof(T);
            return {};
        }

        template<typename T>
        Option<ReadError> skip(const size_t n, const ReadError orElseErr) {
            const auto length = n * sizeof(T);
            if (offset + length > data.size())
                return orElseErr;

            offset += length;
            return {};
        }

        template<typename T>
        Option<ReadError> skip(const ReadError orElseErr) {
            return skip<T>(1, orElseErr);
        }

        template<typename Version>
        ReadResult<Version> deserializeVersion(const Version latest) {
            const auto versionNumber = Try(readByte(EXPECTED_VERSION));

            if (versionNumber > static_cast<uint8_t>(latest))
                return Err(INVALID_VERSION);

            return static_cast<Version>(versionNumber);
        }

        Option<ReadError> validateZVCRFilePrefix(const std::string& prefix);

        ReadResult<DimensionType> deserializeDimensionType();

        ReadResult<PackedSnapshot<SegmentAtom>> deserializePackedSnapshot(size_t snapshotLength);

        Option<ReadError> deserializePaletteTable();

        Option<ReadError> skipPackedSnapshot();

        ReadResult<PackedDeltaData<SegmentAtom>> deserializePackedDeltaData(size_t snapshotLength);

        ReadResult<SegmentState> deserializeSegmentState();

        ReadResult<TileEntityCountInfo> deserializeTileEntityCountInfo();

        ReadResult<SegmentInfo> deserializeSegmentInfo();

        ReadResult<Segment3d> deserializeSegment3d();

        ReadResult<Option<Segment3d>> deserializeOptSegment3d();

        ReadResult<Region3d> deserializeRegion3d(ZVCR3Version version);

        ReadResult<Layer2d> deserializeLayer(size_t snapshotSize);

        ReadResult<LayerContainer2d> deserializeLayers(size_t snapshotSize);

        ReadResult<LayerContainer2d> deserializeBlockLayers();

        ReadResult<LayerContainer2d> deserializeBiomeLayers();

        ReadResult<Segment2d> deserializeSegment2d();

        ReadResult<Option<Segment2d>> deserializeOptSegment2d();

        ReadResult<Region2d> deserializeRegion2d(ZVCR2Version version);
    };

    void serializeZVCR2File(const ZVCR2File& file, WriteHandle& handle);

    void serializeZVCR3File(const ZVCR3File& file, WriteHandle& handle);

    ReadResult<ZVCR2File> deserializeZVCR2File(ReadHandle& handle);

    ReadResult<ZVCR3File> deserializeZVCR3File(ReadHandle& handle);

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
        WriteHandle handle{};
        DefaultSerialization<R>::serialize(file, handle);
        const auto bytesCompressed = compressData(handle.data, zstdCompressionLevel, zstdCompressionThreads);

        std::ofstream fileStream(filepath, std::ios::out | std::ios::binary);
        fileStream.write(reinterpret_cast<const char*>(bytesCompressed.data()), static_cast<int64_t>(bytesCompressed.size()));
        fileStream.close();

        return bytesCompressed.size();
    }

    template<typename R>
    ReadResult<R> readZVCRFile(const fs::path& filepath, const size_t maxDeltas = 0) {
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
            ReadHandle handle{bytesUncompressed, maxDeltas};

            delete[] bytesCompressed;
            return DefaultSerialization<R>::deserialize(handle);
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
    ReadResult<R> readZVCRFileAt(const fs::path& parentDirectory, const RegionLocation& location, const size_t maxDeltas = 0) {
        return readZVCRFile<R>(location.getFilePath(parentDirectory, DefaultSerialization<R>::format), maxDeltas);
    }

}
