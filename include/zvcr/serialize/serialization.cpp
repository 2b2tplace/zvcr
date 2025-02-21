#include <fstream>
#include <filesystem>
#include <ranges>
#include <cstring>
#include <zvcr/common/definitions.hpp>
#include <zvcr/serialize/compression.hpp>
#include <zvcr/serialize/serialization.hpp>

namespace zvcr::serialize::serialization {
    ZVCRResult<DimensionType> deserializeDimensionType(const std::vector<uint8_t>& data, size_t& offset) {
        if (offset >= data.size())
            return Error(EXPECTED_DIMENSION_TYPE);

        const auto dimensionTypeId = data[offset++];

        if (dimensionTypeId > static_cast<uint8_t>(DimensionType::THE_END))
            return Error(INVALID_DIMENSION_TYPE);

        return static_cast<DimensionType>(dimensionTypeId);
    }

    template<typename Version>
    ZVCRResult<Version> deserializeVersion(const std::vector<uint8_t>& data, size_t& offset, const Version latest) {
        if (offset >= data.size())
            return Error(EXPECTED_VERSION);

        const auto versionNumber = data[offset++];

        if (versionNumber > static_cast<uint8_t>(latest))
            return Error(INVALID_VERSION);

        return static_cast<Version>(versionNumber);
    }

    std::optional<ZVCRError> validateZVCRFilePrefix(const std::vector<uint8_t>& data, size_t& offset, const std::string& prefix) {
        if (data.size() < prefix.size())
            return MISSING_HEADER;

        for (size_t i = 0; i < prefix.size(); ++i) {
            if (data[i] != static_cast<uint8_t>(prefix[i]))
                return INVALID_HEADER_PREFIX;
        }
        offset += prefix.size();
        return std::nullopt;
    }

    template<typename R>
    size_t writeZVCRFile(const R& file, const std::string& filename, const ZVCRFileSerialize<R>& serialize,
                      const int zstdCompressionLevel, const int zstdCompressionThreads) {
        std::vector<uint8_t> bytesUncompressed;
        serialize(file, bytesUncompressed);
        const auto bytesCompressed = compression::compressData(bytesUncompressed, zstdCompressionLevel, zstdCompressionThreads);

        std::ofstream fileStream(filename, std::ios::out | std::ios::binary);
        fileStream.write(reinterpret_cast<const char*>(bytesCompressed.data()), static_cast<int64_t>(bytesCompressed.size()));
        fileStream.close();

        return bytesCompressed.size();
    }

    template<typename R>
    ZVCRResult<R> readZVCRFile(const std::string& filename, const size_t maxDeltas, const ZVCRFileDeserialize<R>& deserialize) {
        if (!std::filesystem::exists(filename))
            return Error(FILE_NOT_FOUND);

        try {
            std::ifstream fileStream(filename, std::ios::in | std::ios::binary);

            fileStream.seekg(0, std::ios::end);
            const int64_t fileSize = fileStream.tellg();
            fileStream.seekg(0, std::ios::beg);

            const auto bytesCompressed = new char[fileSize];
            fileStream.read(bytesCompressed, fileSize);
            fileStream.close();

            const auto bytesCompressedVector = std::vector<uint8_t>(bytesCompressed, bytesCompressed + fileSize);
            const auto bytesUncompressed = compression::decompressData(bytesCompressedVector);
            size_t offset{};

            delete[] bytesCompressed;
            return deserialize(bytesUncompressed, offset, maxDeltas);
        } catch (const std::length_error&) {
            return Error(GENERIC_READ_ERROR);
        }
    }

    void serializeBlockStatesSnapshot(const BlockStatesSnapshot& snapshot, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
        data.resize(data.size() + sizeof(time_t));
        std::memcpy(data.data() + data.size() - sizeof(time_t), &snapshot.timestamp, sizeof(time_t));

        const auto& packedData = snapshot.data.packedData;
        const uint64_t packedLength = packedData.size();
        const uint64_t packedSize = packedLength * sizeof(uint64_t);

        data.resize(data.size() + sizeof(uint64_t) + packedLength * sizeof(uint64_t));
        std::memcpy(data.data() + data.size() - sizeof(uint64_t) - packedSize, &packedLength, sizeof(uint64_t));
        std::memcpy(data.data() + data.size() - packedSize, packedData.data(), packedSize);

        const auto& palette = snapshot.data.palette;
        size_t paletteIndex = paletteTable.size();
        for (size_t i = 0; i < paletteTable.size(); ++i) {
            if (const auto& existingPalette = paletteTable[i]; palette == existingPalette) {
                paletteIndex = i;
                break;
            }
        }
        if (paletteIndex == paletteTable.size())
            paletteTable.push_back(palette);

        data.resize(data.size() + sizeof(uint32_t));
        std::memcpy(data.data() + data.size() - sizeof(uint32_t), &paletteIndex, sizeof(uint32_t));
    }

    ZVCRResult<BlockStatesSnapshot> deserializeBlockStatesSnapshot(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const size_t snapshotLength) {
        if (offset + sizeof(time_t) > data.size())
            return Error(EXPECTED_TIMESTAMP);

        time_t timestamp;
        std::memcpy(&timestamp, data.data() + offset, sizeof(time_t));
        offset += sizeof(time_t);

        if (offset + sizeof(uint64_t) > data.size())
            return Error(EXPECTED_PACKED_LENGTH);

        uint64_t packedLength;
        std::memcpy(&packedLength, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);

        if (offset + packedLength * sizeof(uint64_t) > data.size())
            return Error(EXPECTED_PACKED_DATA);

        LongArray packedData(packedLength);
        std::memcpy(packedData.data(), data.data() + offset, packedLength * sizeof(uint64_t));
        offset += packedLength * sizeof(uint64_t);

        if (offset + sizeof(uint32_t) > data.size())
            return Error(EXPECTED_PALETTE_INDEX);

        uint32_t paletteIndex;
        std::memcpy(&paletteIndex, data.data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        const auto& palette = paletteTable[paletteIndex];
        return BlockStatesSnapshot {
            std::move(BlockStates(palette, packedData, snapshotLength)),
                timestamp
            };
    }

    void serializePaletteTable(const std::vector<Palette>& paletteTable, std::vector<uint8_t>& data) {
        data.resize(data.size() + sizeof(uint32_t));
        const uint32_t paletteTableLength = paletteTable.size();
        std::memcpy(data.data() + data.size() - sizeof(uint32_t), &paletteTableLength, sizeof(uint32_t));

        for (const auto& palette : paletteTable) {
            const auto paletteLength = palette.size();
            const size_t paletteSize = paletteLength * sizeof(uint16_t);

            data.resize(data.size() + sizeof(uint16_t) + paletteSize);
            std::memcpy(data.data() + data.size() - sizeof(uint16_t) - paletteSize, &paletteLength, sizeof(uint16_t));
            std::memcpy(data.data() + data.size() - paletteSize, palette.data(), paletteSize);
        }
    }

    ZVCRResult<std::vector<Palette>> deserializePaletteTable(const std::vector<uint8_t>& data, size_t& offset) {
        if (offset + sizeof(uint32_t) > data.size())
            return Error(EXPECTED_PALETTE_TABLE_LENGTH);

        uint32_t paletteTableLength;
        std::memcpy(&paletteTableLength, data.data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        std::vector<Palette> paletteTable;
        for (uint32_t i = 0; i < paletteTableLength; ++i) {
            if (offset + sizeof(uint16_t) > data.size())
                return Error(EXPECTED_PALETTE_LENGTH);

            uint16_t paletteLength;
            std::memcpy(&paletteLength, data.data() + offset, sizeof(uint16_t));
            offset += sizeof(uint16_t);

            const auto paletteLengthSize = static_cast<size_t>(paletteLength);
            if (offset + paletteLengthSize * sizeof(uint16_t) > data.size())
                return Error(EXPECTED_PALETTE_DATA);

            Palette palette;
            palette.resize(paletteLengthSize);
            std::memcpy(palette.data(), data.data() + offset, paletteLengthSize * sizeof(uint16_t));
            offset += paletteLengthSize * sizeof(uint16_t);
            paletteTable.emplace_back(std::move(palette));
        }
        return paletteTable;
    }

    std::optional<ZVCRError> skipBlockStatesSnapshot(const std::vector<uint8_t>& data, size_t& offset) {
        if (offset + sizeof(time_t) > data.size())
            return EXPECTED_TIMESTAMP;

        offset += sizeof(time_t);

        if (offset + sizeof(uint64_t) > data.size())
            return EXPECTED_PACKED_LENGTH;

        uint64_t packedLength;
        std::memcpy(&packedLength, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);

        if (offset + packedLength * sizeof(uint64_t) > data.size())
            return EXPECTED_PACKED_DATA;

        offset += packedLength * sizeof(uint64_t);

        if (offset + sizeof(uint32_t) > data.size())
            return EXPECTED_PALETTE_INDEX;

        uint32_t paletteLength;
        std::memcpy(&paletteLength, data.data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        return std::nullopt;
    }

    void serializeBlockStates(const DeltaBlockStates& section3d, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
        const uint64_t deltaLength = section3d.reverseDeltas.size();
        data.resize(data.size() + sizeof(uint64_t));
        std::memcpy(data.data() + data.size() - sizeof(uint64_t), &deltaLength, sizeof(uint64_t));

        for (const BlockStatesSnapshot& foo : section3d.reverseDeltas)
            serializeBlockStatesSnapshot(foo, data, paletteTable);
    }

    ZVCRResult<DeltaBlockStates> deserializeBlockStates(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const size_t maxDeltas, const size_t snapshotLength) {
        if (offset + sizeof(uint64_t) > data.size())
            return Error(EXPECTED_DELTA_LENGTH);

        size_t deltaLength;
        std::memcpy(&deltaLength, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        std::vector<BlockStatesSnapshot> reverseDeltas;
        reverseDeltas.reserve(deltaLength);

        for (size_t deltaIndex = 0; deltaIndex < deltaLength; ++deltaIndex) {
            if (maxDeltas != 0 && deltaIndex >= maxDeltas) {
                Propagate(skipBlockStatesSnapshot(data, offset));
                continue;
            }
            reverseDeltas.push_back(std::move(Try(deserializeBlockStatesSnapshot(data, offset, paletteTable, snapshotLength))));
        }
        return DeltaBlockStates(reverseDeltas, snapshotLength);
    }

    void serializeSegmentState(const SegmentState& segmentState, std::vector<uint8_t>& data) {
        data.push_back(static_cast<uint8_t>(segmentState.type));
        data.resize(data.size() + sizeof(time_t));
        std::memcpy(data.data() + data.size() - sizeof(time_t), &segmentState.timestamp, sizeof(time_t));
    }

    ZVCRResult<SegmentState> deserializeSegmentState(const std::vector<uint8_t>& data, size_t& offset) {
        if (offset >= data.size())
            return Error(EXPECTED_SEGMENT_STATE_TYPE);

        const auto type = static_cast<SegmentStateType>(data[offset++]);

        if (offset + sizeof(time_t) > data.size())
            return Error(EXPECTED_SEGMENT_STATE_TIMESTAMP);

        time_t timestamp;
        std::memcpy(&timestamp, data.data() + offset, sizeof(time_t));
        offset += sizeof(time_t);

        return SegmentState {type, timestamp};
    }

    void serializeTileEntityCountInfo(const TileEntityCountInfo& tileEntityCounts, std::vector<uint8_t>& data) {
        data.resize(data.size() + sizeof(uint16_t) * TOTAL_TILE_ENTITIES);
        std::memcpy(data.data() + data.size() - sizeof(uint16_t) * TOTAL_TILE_ENTITIES, tileEntityCounts.counts.data(), sizeof(uint16_t) * TOTAL_TILE_ENTITIES);
        data.resize(data.size() + sizeof(time_t));
        std::memcpy(data.data() + data.size() - sizeof(time_t), &tileEntityCounts.timestamp, sizeof(time_t));
    }

    ZVCRResult<TileEntityCountInfo> deserializeTileEntityCountInfo(const std::vector<uint8_t>& data, size_t& offset) {
        if (offset + sizeof(uint16_t) * TOTAL_TILE_ENTITIES > data.size())
            return Error(EXPECTED_TILE_ENTITY_COUNTS);

        std::vector<uint16_t> counts(TOTAL_TILE_ENTITIES);
        std::memcpy(counts.data(), data.data() + offset, sizeof(uint16_t) * TOTAL_TILE_ENTITIES);
        offset += sizeof(uint16_t) * TOTAL_TILE_ENTITIES;

        if (offset + sizeof(time_t) > data.size())
            return Error(EXPECTED_TILE_ENTITY_COUNTS_TIMESTAMP);

        time_t timestamp;
        std::memcpy(&timestamp, data.data() + offset, sizeof(time_t));
        offset += sizeof(time_t);

        return TileEntityCountInfo {std::move(counts), timestamp};
    }

    void serializeSegmentInfo(const SegmentInfo& segmentInfo, std::vector<uint8_t>& data) {
        const uint64_t statesLength = segmentInfo.segmentStates.size();
        data.resize(data.size() + sizeof(uint64_t));
        std::memcpy(data.data() + data.size() - sizeof(uint64_t), &statesLength, sizeof(uint64_t));

        for (const SegmentState& state : segmentInfo.segmentStates)
            serializeSegmentState(state, data);

        const size_t tileEntityCountsLength = segmentInfo.tileEntityCounts.size();
        data.resize(data.size() + sizeof(uint64_t));
        std::memcpy(data.data() + data.size() - sizeof(uint64_t), &tileEntityCountsLength, sizeof(uint64_t));

        for (const TileEntityCountInfo& tiles : segmentInfo.tileEntityCounts)
            serializeTileEntityCountInfo(tiles, data);
    }

    ZVCRResult<SegmentInfo> deserializeSegmentInfo(const std::vector<uint8_t>& data, size_t& offset) {
        if (offset + sizeof(uint64_t) > data.size())
            return Error(EXPECTED_SEGMENT_STATES_LENGTH);

        uint64_t statesLength;
        std::memcpy(&statesLength, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);

        SegmentStates states;
        states.reserve(statesLength);

        for (size_t stateIndex = 0; stateIndex < statesLength; ++stateIndex)
            states.push_back(Try(deserializeSegmentState(data, offset)));

        if (offset + sizeof(uint64_t) > data.size())
            return Error(EXPECTED_TILE_ENTITIES_LENGTH);

        uint64_t tileEntitiesLength;
        std::memcpy(&tileEntitiesLength, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);

        TileEntityCounts tileEntityCounts;
        tileEntityCounts.reserve(tileEntitiesLength);

        for (size_t tileEntityIndex = 0; tileEntityIndex < tileEntitiesLength; ++tileEntityIndex)
            tileEntityCounts.push_back(std::move(Try(deserializeTileEntityCountInfo(data, offset))));

        return SegmentInfo(states, tileEntityCounts);
    }

    void serializeSegment3d(const Segment3d& segment3d, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
        for (const DeltaBlockStates& section : segment3d.sections)
            serializeBlockStates(section, data, paletteTable);

        serializeSegmentInfo(segment3d.info, data);
    }

    ZVCRResult<Segment3d> deserializeSegment3d(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const size_t maxDeltas, const uint32_t sectionAmount) {
        Sections3d sections;
        sections.reserve(sectionAmount);

        for (size_t sectionIndex = 0; sectionIndex < sectionAmount; ++sectionIndex)
            sections.push_back(std::move(Try(deserializeBlockStates(data, offset, paletteTable, maxDeltas, SECTION_3D_SIZE_BLOCKS))));

        const auto segmentInfo = Try(deserializeSegmentInfo(data, offset));
        return Segment3d(sections, segmentInfo);
    }

    void serializeOptSegment3d(const std::optional<Segment3d>& segment3dOpt, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
        if (!segment3dOpt.has_value()) {
            data.push_back(0);
            return;
        }
        data.push_back(1);
        serializeSegment3d(segment3dOpt.value(), data, paletteTable);
    }

    ZVCRResult<std::optional<Segment3d>> deserializeOptSegment3d(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const size_t maxDeltas, const uint32_t sectionAmount) {
        if (offset >= data.size())
            return Error(EXPECTED_SEGMENT_INDICATOR);

        if (data[offset++] == 0)
            return static_cast<ZVCRResult<std::optional<Segment3d>>>(std::nullopt);

        return static_cast<ZVCRResult<std::optional<Segment3d>>>(Try(deserializeSegment3d(data, offset, paletteTable, maxDeltas, sectionAmount)));
    }

    void serializeRegion3d(const Region3d& region, std::vector<uint8_t>& data) {
        std::vector<Palette> paletteTable;
        std::vector<uint8_t> regionData;

        for (const auto& segment3d : region.segments)
            serializeOptSegment3d(segment3d, regionData, paletteTable);

        serializePaletteTable(paletteTable, data);
        data.insert(data.end(), regionData.begin(), regionData.end());
    }

    ZVCRResult<Region3d> deserializeRegion3d(const std::vector<uint8_t>& data, size_t& offset, const size_t maxDeltas, const uint32_t sectionAmount) {
        const auto paletteTable = Try(deserializePaletteTable(data, offset));
        Segments3d segment3ds(SEGMENTS_PER_REGION);
        for (size_t segment3dIndex = 0; segment3dIndex < SEGMENTS_PER_REGION; ++segment3dIndex)
            segment3ds[segment3dIndex] = std::move(Try(deserializeOptSegment3d(data, offset, paletteTable, maxDeltas, sectionAmount)));

        return Region3d(segment3ds);
    }

    void serializeZVCR3File(const ZVCR3File& file, std::vector<uint8_t>& data) {
        const std::string prefix = ZVCR3_FILE_PREFIX;
        data.insert(data.end(), prefix.begin(), prefix.end());
        data.push_back(static_cast<uint8_t>(file.version));
        data.push_back(static_cast<uint8_t>(file.dimensionType));
        serializeRegion3d(file.region, data);
    }

    ZVCRResult<ZVCR3File> deserializeZVCR3File(const std::vector<uint8_t>& data, size_t& offset, const size_t maxDeltas) {
        validateZVCRFilePrefix(data, offset, ZVCR3_FILE_PREFIX);
        const auto version = Try(deserializeVersion(data, offset, ZVCR3_VER_LATEST));
        const auto dimensionType = Try(deserializeDimensionType(data, offset));

        const auto sectionAmount = DimensionTypePropertyRegistry.at(dimensionType).height / SEGMENT_SIDELENGTH_BLOCKS;
        const auto region = Try(deserializeRegion3d(data, offset, maxDeltas, sectionAmount));

        return ZVCR3File {version, dimensionType, region};
    }

    size_t writeZVCR3File(const ZVCR3File& file, const std::string& filename, const int zstdCompressionLevel, const int zstdCompressionThreads) {
        return writeZVCRFile(file, filename, ZVCRFileSerialize(serializeZVCR3File), zstdCompressionLevel, zstdCompressionThreads);
    }

    ZVCRResult<ZVCR3File> readZVCR3File(const std::string& filename, const size_t maxDeltas) {
        return readZVCRFile(filename, maxDeltas, ZVCRFileDeserialize(deserializeZVCR3File));
    }

    void serializeLayer(const Layer2d& layer, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
        data.push_back(layer.type);
        serializeBlockStates(layer.deltas, data, paletteTable);
    }

    ZVCRResult<Layer2d> deserializeLayer(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const size_t maxDeltas) {
        if (offset >= data.size())
            return Error(EXPECTED_LAYER_TYPE);

        const auto type = data[offset++];
        const auto deltas = Try(deserializeBlockStates(data, offset, paletteTable, maxDeltas, SECTION_2D_SIZE_BLOCKS));

        return Layer2d {deltas, type};
    }

    void serializeLayers(const Layers2d& layers, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
        const uint64_t layersLength = layers.size();
        data.resize(data.size() + sizeof(uint64_t));
        std::memcpy(data.data() + data.size() - sizeof(uint64_t), &layersLength, sizeof(uint64_t));

        for (const auto &layer: layers | std::views::values)
            serializeLayer(layer, data, paletteTable);
    }

    ZVCRResult<Layers2d> deserializeLayers(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const size_t maxDeltas) {
        if (offset + sizeof(uint64_t) > data.size())
            return Error(EXPECTED_LAYERS_LENGTH);

        uint64_t layersLength;
        std::memcpy(&layersLength, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);

        Layers2d layers;
        for (size_t layerIndex = 0; layerIndex < layersLength; ++layerIndex) {
            const auto layer = Try(deserializeLayer(data, offset, paletteTable, maxDeltas));
            layers[layer.type] = layer;
        }
        return layers;
    }

    void serializeSegment2d(const Segment2d& segment, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
        serializeLayers(segment.layers, data, paletteTable);
        serializeSegmentInfo(segment.info, data);
    }

    ZVCRResult<Segment2d> deserializeSegment2d(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const size_t maxDeltas) {
        return Segment2d {
            Try(deserializeLayers(data, offset, paletteTable, maxDeltas)),
            Try(deserializeSegmentInfo(data, offset))
        };
    }

    void serializeOptSegment2d(const std::optional<Segment2d>& segment, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
        if (!segment.has_value()) {
            data.push_back(0);
            return;
        }
        data.push_back(1);
        serializeSegment2d(segment.value(), data, paletteTable);
    }

    ZVCRResult<std::optional<Segment2d>> deserializeOptSegment2d(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const size_t maxDeltas) {
        if (offset >= data.size())
            return Error(EXPECTED_SEGMENT_INDICATOR);

        if (data[offset++] == 0)
            return static_cast<ZVCRResult<std::optional<Segment2d>>>(std::nullopt);

        return static_cast<ZVCRResult<std::optional<Segment2d>>>(Try(deserializeSegment2d(data, offset, paletteTable, maxDeltas)));
    }

    void serializeRegion2d(const Region2d& region, std::vector<uint8_t>& data) {
        std::vector<Palette> paletteTable;
        std::vector<uint8_t> regionData;

        for (const auto& segment : region.segments)
            serializeOptSegment2d(segment, regionData, paletteTable);

        serializePaletteTable(paletteTable, data);
        data.insert(data.end(), regionData.begin(), regionData.end());
    }

    ZVCRResult<Region2d> deserializeRegion2d(const std::vector<uint8_t>& data, size_t& offset, const size_t maxDeltas) {
        const auto paletteTable = Try(deserializePaletteTable(data, offset));
        Segments2d segment3ds(SEGMENTS_PER_REGION);
        for (size_t segmentIndex = 0; segmentIndex < SEGMENTS_PER_REGION; ++segmentIndex)
            segment3ds[segmentIndex] = Try(deserializeOptSegment2d(data, offset, paletteTable, maxDeltas));

        return Region2d(segment3ds);
    }

    void serializeZVCR2File(const ZVCR2File& file, std::vector<uint8_t>& data) {
        const std::string prefix = ZVCR2_FILE_PREFIX;
        data.insert(data.end(), prefix.begin(), prefix.end());
        data.push_back(static_cast<uint8_t>(file.version));
        data.push_back(static_cast<uint8_t>(file.dimensionType));
        serializeRegion2d(file.region, data);
    }

    ZVCRResult<ZVCR2File> deserializeZVCR2File(const std::vector<uint8_t>& data, size_t& offset, const size_t maxDeltas) {
        validateZVCRFilePrefix(data, offset, ZVCR2_FILE_PREFIX);
        const auto version = Try(deserializeVersion(data, offset, ZVCR2_VER_LATEST));
        const auto dimensionType = Try(deserializeDimensionType(data, offset));
        const auto region = Try(deserializeRegion2d(data, offset, maxDeltas));

        return ZVCR2File {version, dimensionType, region};
    }

    size_t writeZVCR2File(const ZVCR2File& file, const std::string& filename, const int zstdCompressionLevel, const int zstdCompressionThreads) {
        return writeZVCRFile(file, filename, ZVCRFileSerialize(serializeZVCR2File), zstdCompressionLevel, zstdCompressionThreads);
    }

    ZVCRResult<ZVCR2File> readZVCR2File(const std::string& filename, const size_t maxDeltas) {
        return readZVCRFile(filename, maxDeltas, ZVCRFileDeserialize(deserializeZVCR2File));
    }
}
