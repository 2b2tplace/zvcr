#pragma once

#include <functional>
#include <fstream>
#include <zvcr/serialize/compression.hpp>
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

    using namespace reverse_delta;
    using namespace region;
    using namespace paletted_storage;
    using namespace definitions;

    using Palette = std::vector<SegmentAtom>;

    static constexpr auto MAX_DELTA_LENGTH = 65536;
    static constexpr auto MAX_SEGMENT_STATES_LENGTH = 65536;
    static constexpr auto MAX_TILE_ENTITIES_LENGTH = 65536;

    static constexpr auto MAX_PACKED_LENGTH = 1048560;
    static constexpr auto MAX_PALETTE_TABLE_LENGTH = 262144;

    enum ReadErrorType {
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
        INVALID_PALETTE_INDEX,
        INVALID_DELTA_LENGTH,
        INVALID_PACKED_LENGTH,
        INVALID_PALETTE_TABLE_LENGTH,
        INVALID_SEGMENT_STATES_LENGTH,
        INVALID_TILE_ENTITIES_LENGTH,
    };

    class ReadHandle;

    static constexpr auto DBG_SLICE_SIZE = 32;

    struct ReadError {
        ReadErrorType type;
        size_t offset;
        std::string message;
        std::vector<uint8_t> dumpSlice;
        size_t sliceStart;
        size_t sliceEnd;
        size_t dumpPoint;
        size_t dataLength;

        [[nodiscard]]
        std::string what() const;

        void attach(const ReadHandle& handle);
    };

    static constexpr auto ZSTD_COMPRESSION_LEVEL_DEFAULT = 12;
    static constexpr auto ZSTD_COMPRESSION_THREADS_DEFAULT = 4;

    namespace fs = std::filesystem;

    template<typename R>
    using ReadResult = std::expected<R, ReadError>;

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

            if (protocolVersion == 0 && version == ZVCR3Version::ZVCR3_0_0_0_1)
                protocolVersion = PROTOCOL_VERSION_ZVCR_0_0_0_X;
        }

        void initialize(const ZVCR2Version version) {
            supportBiomes = version >= ZVCR2Version::ZVCR2_0_1_0_0;
            supportDynamicVersioning = version >= ZVCR2Version::ZVCR2_0_1_1_0;

            if (protocolVersion == 0 && version == ZVCR2Version::ZVCR2_0_0_0_0)
                protocolVersion = PROTOCOL_VERSION_ZVCR_0_0_0_X;
        }
    };

    class WriteHandle {
        std::vector<Palette> paletteTableStorage;

    public:
        Context ctx;
        std::vector<uint8_t> data;

        template<typename T>
        void write(const T& value) {
            data.resize(data.size() + sizeof(T));
            std::memcpy(data.data() + data.size() - sizeof(T), &value, sizeof(T));
        }

        template<typename T>
        void writeArray(const T* array, const size_t length) {
            const auto size = length * sizeof(T);
            data.resize(data.size() + size);
            std::memcpy(data.data() + data.size() - size, array, size);
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

        void serializeRegion3d(const Region3d& region);

        void serializeLayer(const Layer2d& layer);

        void serializeLayers(const LayerContainer2d& layers);

        void serializeSegment2d(const Segment2d& segment);

        void serializeOptSegment2d(const Option<Segment2d>& segment);

        void serializeRegion2d(const Region2d& region);
    };

    class ReadHandle {
        size_t offset{};
        std::vector<Palette> paletteTable{};
        uint32_t sectionCount{};

        size_t maxDeltas;

    public:
        Context ctx{};
        const std::vector<uint8_t>& data;

        explicit ReadHandle(const std::vector<uint8_t>& data, const size_t maxDeltas):
            maxDeltas(maxDeltas), data(data) {}

        [[nodiscard]]
        size_t getOffset() const {
            return offset;
        }

        [[nodiscard]]
        ReadResult<uint8_t> readByte(const ReadErrorType orElseErr) {
            if (offset >= data.size()) {
                const auto err = ReadError{orElseErr, offset, "Read out of bounds"};
                return Err(err);
            }
            return data[offset++];
        }

        template<typename T>
        [[nodiscard]]
        ReadResult<T> read(const ReadErrorType orElseErr) {
            if (offset + sizeof(T) > data.size()) {
                const auto err = ReadError{orElseErr, offset, "Read out of bounds"};
                return Err(err);
            }
            T value;
            std::memcpy(&value, data.data() + offset, sizeof(T));
            offset += sizeof(T);
            return value;
        }

        template<typename T>
        [[nodiscard]]
        Option<ReadError> readArray(std::vector<T>& array, const ReadErrorType orElseErr) {
            const auto length = array.size();
            if (offset + length * sizeof(T) > data.size())
                return ReadError{orElseErr, offset, "Read out of bounds"};

            std::memcpy(array.data(), data.data() + offset, length * sizeof(T));
            offset += length * sizeof(T);
            return None;
        }

        template<typename T>
        [[nodiscard]]
        Option<ReadError> skip(const size_t n, const ReadErrorType orElseErr) {
            const auto length = n * sizeof(T);
            if (offset + length > data.size())
                return ReadError{orElseErr, offset, "Read out of bounds"};

            offset += length;
            return None;
        }

        template<typename T>
        [[nodiscard]]
        Option<ReadError> skip(const ReadErrorType orElseErr) {
            return skip<T>(1, orElseErr);
        }

        template<typename Version>
        [[nodiscard]]
        ReadResult<Version> deserializeVersion(const Version latest) {
            const auto versionNumber = Try(readByte(EXPECTED_VERSION));

            if (versionNumber > static_cast<uint8_t>(latest)) {
                const auto err = ReadError{INVALID_VERSION, offset, "Read out of bounds"};
                return Err(err);
            }
            return static_cast<Version>(versionNumber);
        }

        [[nodiscard]]
        Option<ReadError> validateZVCRFilePrefix(const std::string& prefix);

        [[nodiscard]]
        ReadResult<DimensionType> deserializeDimensionType();

        [[nodiscard]]
        ReadResult<PackedSnapshot<SegmentAtom>> deserializePackedSnapshot(size_t snapshotLength);

        [[nodiscard]]
        Option<ReadError> deserializePaletteTable();

        [[nodiscard]]
        Option<ReadError> skipPackedSnapshot();

        [[nodiscard]]
        ReadResult<PackedDeltaData<SegmentAtom>> deserializePackedDeltaData(size_t snapshotLength);

        [[nodiscard]]
        ReadResult<SegmentState> deserializeSegmentState();

        [[nodiscard]]
        ReadResult<TileEntityCountInfo> deserializeTileEntityCountInfo();

        [[nodiscard]]
        ReadResult<SegmentInfo> deserializeSegmentInfo();

        [[nodiscard]]
        ReadResult<Segment3d> deserializeSegment3d();

        [[nodiscard]]
        ReadResult<Option<Segment3d>> deserializeOptSegment3d();

        [[nodiscard]]
        ReadResult<Region3d> deserializeRegion3d(ZVCR3Version version);

        [[nodiscard]]
        ReadResult<Layer2d> deserializeLayer(size_t snapshotSize);

        [[nodiscard]]
        ReadResult<LayerContainer2d> deserializeLayers(size_t snapshotSize);

        [[nodiscard]]
        ReadResult<LayerContainer2d> deserializeBlockLayers();

        [[nodiscard]]
        ReadResult<LayerContainer2d> deserializeBiomeLayers();

        [[nodiscard]]
        ReadResult<Segment2d> deserializeSegment2d();

        [[nodiscard]]
        ReadResult<Option<Segment2d>> deserializeOptSegment2d();

        [[nodiscard]]
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
                         const uint16_t protocolVersion,
                         const int zstdCompressionLevel = ZSTD_COMPRESSION_LEVEL_DEFAULT,
                         const int zstdCompressionThreads = ZSTD_COMPRESSION_LEVEL_DEFAULT) {
        WriteHandle handle{};
        handle.ctx.protocolVersion = protocolVersion;

        DefaultSerialization<R>::serialize(file, handle);
        const auto bytesCompressed = compressData(handle.data, zstdCompressionLevel, zstdCompressionThreads);

        std::ofstream fileStream(filepath, std::ios::out | std::ios::binary);
        fileStream.write(reinterpret_cast<const char*>(bytesCompressed.data()), static_cast<int64_t>(bytesCompressed.size()));
        fileStream.close();

        return bytesCompressed.size();
    }

    template<typename R>
    ReadResult<R> readZVCRFile(const fs::path& filepath, uint16_t* protocolVersion = nullptr, const size_t maxDeltas = 0) {
        if (!exists(filepath)) {
            static const auto err = ReadError{FILE_NOT_FOUND, 0, "File not found"};
            return Err(err);
        }
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
        try {
            const auto result = DefaultSerialization<R>::deserialize(handle);
            if (!result.has_value()) {
                auto err = result.error();
                err.attach(handle);
                return Err(err);
            }
            if (protocolVersion != nullptr)
                *protocolVersion = handle.ctx.protocolVersion;

            return result.value();
        } catch (const std::length_error& e) {
            auto err = ReadError{GENERIC_READ_ERROR, handle.getOffset(), "Generic read error: " + std::string(e.what())};
            err.attach(handle);
            return Err(err);
        }
    }

    template<typename R>
    size_t writeZVCRFileAt(const R& file, const fs::path& parentDirectory, const RegionLocation& location,
                           const uint16_t protocolVersion,
                           const int zstdCompressionLevel = ZSTD_COMPRESSION_LEVEL_DEFAULT,
                           const int zstdCompressionThreads = ZSTD_COMPRESSION_LEVEL_DEFAULT) {
        create_directories(location.getDirectory(parentDirectory));
        return writeZVCRFile<R>(file, location.getFilePath(parentDirectory, DefaultSerialization<R>::format),
                                protocolVersion, zstdCompressionLevel, zstdCompressionThreads);
    }

    template<typename R>
    ReadResult<R> readZVCRFileAt(const fs::path& parentDirectory, const RegionLocation& location,
                                 uint16_t* protocolVersion = nullptr, const size_t maxDeltas = 0) {
        return readZVCRFile<R>(location.getFilePath(parentDirectory, DefaultSerialization<R>::format), protocolVersion, maxDeltas);
    }

}
