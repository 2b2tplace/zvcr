#include <zvcr/serialize/serialization.hpp>
#include <fstream>
#include <filesystem>
#include <ranges>
#include <cstring>
#include <zvcr/common/definitions.hpp>
#include <zvcr/serialize/compression.hpp>

namespace zvcr::serialize {

    ZVCRResult<DimensionType> deserializeDimensionType(const std::vector<uint8_t>& data, size_t& offset) {
        if (offset >= data.size())
            return Err(EXPECTED_DIMENSION_TYPE);

        const auto dimensionTypeId = data[offset++];

        if (dimensionTypeId > static_cast<uint8_t>(DimensionType::THE_END))
            return Err(INVALID_DIMENSION_TYPE);

        return static_cast<DimensionType>(dimensionTypeId);
    }

    template<typename Version>
    ZVCRResult<Version> deserializeVersion(const std::vector<uint8_t>& data, size_t& offset, const Version latest) {
        if (offset >= data.size())
            return Err(EXPECTED_VERSION);

        const auto versionNumber = data[offset++];

        if (versionNumber > static_cast<uint8_t>(latest))
            return Err(INVALID_VERSION);

        return static_cast<Version>(versionNumber);
    }

    Option<ZVCRError> validateZVCRFilePrefix(const std::vector<uint8_t>& data, size_t& offset, const std::string& prefix) {
        if (data.size() < prefix.size())
            return MISSING_HEADER;

        for (size_t i = 0; i < prefix.size(); ++i) {
            if (data[i] != static_cast<uint8_t>(prefix[i]))
                return INVALID_HEADER_PREFIX;
        }
        offset += prefix.size();
        return {};
    }

    void serializePackedSnapshot(const PackedSnapshot<SegmentAtom>& snapshot, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
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

    ZVCRResult<PackedSnapshot<SegmentAtom>> deserializePackedSnapshot(const std::vector<uint8_t>& data, size_t& offset,
                                                                      const std::vector<Palette>& paletteTable, const size_t snapshotLength) {
        if (offset + sizeof(time_t) > data.size())
            return Err(EXPECTED_TIMESTAMP);

        time_t timestamp;
        std::memcpy(&timestamp, data.data() + offset, sizeof(time_t));
        offset += sizeof(time_t);

        if (offset + sizeof(uint64_t) > data.size())
            return Err(EXPECTED_PACKED_LENGTH);

        uint64_t packedLength;
        std::memcpy(&packedLength, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);

        if (offset + packedLength * sizeof(uint64_t) > data.size())
            return Err(EXPECTED_PACKED_DATA);

        LongArray packedData(packedLength);
        std::memcpy(packedData.data(), data.data() + offset, packedLength * sizeof(uint64_t));
        offset += packedLength * sizeof(uint64_t);

        if (offset + sizeof(uint32_t) > data.size())
            return Err(EXPECTED_PALETTE_INDEX);

        uint32_t paletteIndex;
        std::memcpy(&paletteIndex, data.data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        const auto& palette = paletteTable[paletteIndex];
        return PackedSnapshot {
            PackedData{palette, packedData, snapshotLength},
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
            return Err(EXPECTED_PALETTE_TABLE_LENGTH);

        uint32_t paletteTableLength;
        std::memcpy(&paletteTableLength, data.data() + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        std::vector<Palette> paletteTable;
        for (uint32_t i = 0; i < paletteTableLength; ++i) {
            if (offset + sizeof(uint16_t) > data.size())
                return Err(EXPECTED_PALETTE_LENGTH);

            uint16_t paletteLength;
            std::memcpy(&paletteLength, data.data() + offset, sizeof(uint16_t));
            offset += sizeof(uint16_t);

            const auto paletteLengthSize = static_cast<size_t>(paletteLength);
            if (offset + paletteLengthSize * sizeof(uint16_t) > data.size())
                return Err(EXPECTED_PALETTE_DATA);

            Palette palette;
            palette.resize(paletteLengthSize);
            std::memcpy(palette.data(), data.data() + offset, paletteLengthSize * sizeof(uint16_t));
            offset += paletteLengthSize * sizeof(uint16_t);
            paletteTable.emplace_back(palette);
        }
        return paletteTable;
    }

    Option<ZVCRError> skipPackedSnapshot(const std::vector<uint8_t>& data, size_t& offset) {
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
        return {};
    }

    void serializePackedDeltaData(const PackedDeltaData<SegmentAtom>& section3d, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
        const uint64_t deltaLength = section3d.reverseDeltas.size();
        data.resize(data.size() + sizeof(uint64_t));
        std::memcpy(data.data() + data.size() - sizeof(uint64_t), &deltaLength, sizeof(uint64_t));

        for (const PackedSnapshot<SegmentAtom>& snapshot : section3d.reverseDeltas)
            serializePackedSnapshot(snapshot, data, paletteTable);
    }

    ZVCRResult<PackedDeltaData<SegmentAtom>> deserializePackedDeltaData(const std::vector<uint8_t>& data, size_t& offset,
                                                                        const std::vector<Palette>& paletteTable,
                                                                        const size_t maxDeltas, const size_t snapshotLength) {
        if (offset + sizeof(uint64_t) > data.size())
            return Err(EXPECTED_DELTA_LENGTH);

        size_t deltaLength;
        std::memcpy(&deltaLength, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        std::vector<PackedSnapshot<SegmentAtom>> reverseDeltas;
        reverseDeltas.reserve(deltaLength);

        for (size_t deltaIndex = 0; deltaIndex < deltaLength; ++deltaIndex) {
            if (maxDeltas != 0 && deltaIndex >= maxDeltas) {
                Propagate(skipPackedSnapshot(data, offset));
                continue;
            }
            reverseDeltas.push_back(Try(deserializePackedSnapshot(data, offset, paletteTable, snapshotLength)));
        }
        return PackedDeltaData{reverseDeltas, snapshotLength};
    }

    void serializeSegmentState(const SegmentState& segmentState, std::vector<uint8_t>& data) {
        data.push_back(static_cast<uint8_t>(segmentState.type));
        data.resize(data.size() + sizeof(time_t));
        std::memcpy(data.data() + data.size() - sizeof(time_t), &segmentState.timestamp, sizeof(time_t));
    }

    ZVCRResult<SegmentState> deserializeSegmentState(const std::vector<uint8_t>& data, size_t& offset) {
        if (offset >= data.size())
            return Err(EXPECTED_SEGMENT_STATE_TYPE);

        const auto type = static_cast<SegmentStateType>(data[offset++]);

        if (offset + sizeof(time_t) > data.size())
            return Err(EXPECTED_SEGMENT_STATE_TIMESTAMP);

        time_t timestamp;
        std::memcpy(&timestamp, data.data() + offset, sizeof(time_t));
        offset += sizeof(time_t);

        return SegmentState{type, timestamp};
    }

    void serializeTileEntityCountInfo(const TileEntityCountInfo& tileEntityCounts, std::vector<uint8_t>& data) {
        data.resize(data.size() + sizeof(uint16_t) * TOTAL_TILE_ENTITIES);
        std::memcpy(data.data() + data.size() - sizeof(uint16_t) * TOTAL_TILE_ENTITIES, tileEntityCounts.counts.data(), sizeof(uint16_t) * TOTAL_TILE_ENTITIES);
        data.resize(data.size() + sizeof(time_t));
        std::memcpy(data.data() + data.size() - sizeof(time_t), &tileEntityCounts.timestamp, sizeof(time_t));
    }

    ZVCRResult<TileEntityCountInfo> deserializeTileEntityCountInfo(const std::vector<uint8_t>& data, size_t& offset) {
        if (offset + sizeof(uint16_t) * TOTAL_TILE_ENTITIES > data.size())
            return Err(EXPECTED_TILE_ENTITY_COUNTS);

        std::vector<uint16_t> counts(TOTAL_TILE_ENTITIES);
        std::memcpy(counts.data(), data.data() + offset, sizeof(uint16_t) * TOTAL_TILE_ENTITIES);
        offset += sizeof(uint16_t) * TOTAL_TILE_ENTITIES;

        if (offset + sizeof(time_t) > data.size())
            return Err(EXPECTED_TILE_ENTITY_COUNTS_TIMESTAMP);

        time_t timestamp;
        std::memcpy(&timestamp, data.data() + offset, sizeof(time_t));
        offset += sizeof(time_t);

        return TileEntityCountInfo{counts, timestamp};
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
            return Err(EXPECTED_SEGMENT_STATES_LENGTH);

        uint64_t statesLength;
        std::memcpy(&statesLength, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);

        SegmentStates states;
        states.reserve(statesLength);

        for (size_t stateIndex = 0; stateIndex < statesLength; ++stateIndex)
            states.push_back(Try(deserializeSegmentState(data, offset)));

        if (offset + sizeof(uint64_t) > data.size())
            return Err(EXPECTED_TILE_ENTITIES_LENGTH);

        uint64_t tileEntitiesLength;
        std::memcpy(&tileEntitiesLength, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);

        TileEntityCounts tileEntityCounts;
        tileEntityCounts.reserve(tileEntitiesLength);

        for (size_t tileEntityIndex = 0; tileEntityIndex < tileEntitiesLength; ++tileEntityIndex)
            tileEntityCounts.push_back(Try(deserializeTileEntityCountInfo(data, offset)));

        return SegmentInfo{states, tileEntityCounts};
    }

    void serializeSegment3d(const Segment3d& segment3d, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable,
                            const ZVCR3Version version) {
        for (const PackedDeltaData<SegmentAtom>& section : segment3d.blockSections.getSections())
            serializePackedDeltaData(section, data, paletteTable);

        if (version >= ZVCR3Version::ZVCR3_0_1_0_0) {
            for (const PackedDeltaData<SegmentAtom>& section : segment3d.biomeSections.getSections())
                serializePackedDeltaData(section, data, paletteTable);
        }
        serializeSegmentInfo(segment3d.info, data);
    }

    ZVCRResult<Segment3d> deserializeSegment3d(const std::vector<uint8_t>& data, size_t& offset,
                                               const std::vector<Palette>& paletteTable,
                                               const size_t maxDeltas, const uint32_t sectionCount,
                                               const ZVCR3Version version) {
        Segment3d segment{sectionCount};

        for (size_t sectionIndex = 0; sectionIndex < sectionCount; ++sectionIndex)
            segment.blockSections.getSection(sectionIndex) = Try(deserializePackedDeltaData(data, offset, paletteTable,
                                    maxDeltas, SECTION_3D_SIZE_BLOCKS));

        if (version >= ZVCR3Version::ZVCR3_0_1_0_0) {
            for (size_t sectionIndex = 0; sectionIndex < sectionCount; ++sectionIndex)
                segment.biomeSections.getSection(sectionIndex) = Try(deserializePackedDeltaData(data, offset, paletteTable,
                                        maxDeltas, SECTION_3D_SIZE_BIOMES));
        }
        segment.info = Try(deserializeSegmentInfo(data, offset));
        return segment;
    }

    void serializeOptSegment3d(const Option<Segment3d>& segment3dOpt, std::vector<uint8_t>& data,
                               std::vector<Palette>& paletteTable, const ZVCR3Version version) {
        if (segment3dOpt.none()) {
            data.push_back(0);
            return;
        }
        data.push_back(1);
        serializeSegment3d(segment3dOpt.unwrap(), data, paletteTable, version);
    }

    ZVCRResult<Option<Segment3d>> deserializeOptSegment3d(const std::vector<uint8_t>& data, size_t& offset,
                                                          const std::vector<Palette>& paletteTable,
                                                          const size_t maxDeltas, const uint32_t sectionCount,
                                                          const ZVCR3Version version) {
        if (offset >= data.size())
            return Err(EXPECTED_SEGMENT_INDICATOR);

        if (data[offset++] == 0)
            return Option<Segment3d>();

        return static_cast<ZVCRResult<Option<Segment3d>>>(Try(deserializeSegment3d(data, offset, paletteTable, maxDeltas, sectionCount, version)));
    }

    void serializeRegion3d(const Region3d& region, std::vector<uint8_t>& data, const ZVCR3Version version) {
        std::vector<Palette> paletteTable;
        std::vector<uint8_t> regionData;

        for (const auto& segment3d : region.segments)
            serializeOptSegment3d(segment3d, regionData, paletteTable, version);

        serializePaletteTable(paletteTable, data);
        data.insert(data.end(), regionData.begin(), regionData.end());
    }

    ZVCRResult<Region3d> deserializeRegion3d(const std::vector<uint8_t>& data, size_t& offset,
                                             const size_t maxDeltas, const uint32_t sectionCount,
                                             const ZVCR3Version version) {
        const auto paletteTable = Try(deserializePaletteTable(data, offset));
        Segments3d segment3ds(SEGMENTS_PER_REGION);
        for (size_t segment3dIndex = 0; segment3dIndex < SEGMENTS_PER_REGION; ++segment3dIndex)
            segment3ds[segment3dIndex] = Try(deserializeOptSegment3d(data, offset, paletteTable, maxDeltas, sectionCount, version));

        return Region3d{segment3ds};
    }

    void serializeZVCR3File(const ZVCR3File& file, std::vector<uint8_t>& data) {
        const std::string prefix = ZVCR3_FILE_PREFIX;
        data.insert(data.end(), prefix.begin(), prefix.end());
        data.push_back(static_cast<uint8_t>(file.version));
        data.push_back(static_cast<uint8_t>(file.dimensionType));
        serializeRegion3d(file.region, data, file.version);
    }

    ZVCRResult<ZVCR3File> deserializeZVCR3File(const std::vector<uint8_t>& data, size_t& offset, const size_t maxDeltas) {
        validateZVCRFilePrefix(data, offset, ZVCR3_FILE_PREFIX);
        const auto version = Try(deserializeVersion(data, offset, ZVCR3_VER_LATEST));
        const auto dimensionType = Try(deserializeDimensionType(data, offset));

        const auto sectionCount = getProperties(dimensionType).height / SEGMENT_SIDELENGTH_BLOCKS;
        const auto region = Try(deserializeRegion3d(data, offset, maxDeltas, sectionCount, version));

        return ZVCR3File{version, dimensionType, region};
    }

    void serializeLayer(const Layer2d& layer, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
        data.push_back(layer.type);
        serializePackedDeltaData(layer.deltas, data, paletteTable);
    }

    ZVCRResult<Layer2d> deserializeLayer(const std::vector<uint8_t>& data, size_t& offset,
                                         const std::vector<Palette>& paletteTable, const size_t maxDeltas, const size_t snapshotSize) {
        if (offset >= data.size())
            return Err(EXPECTED_LAYER_TYPE);

        const auto type = data[offset++];
        const auto deltas = Try(deserializePackedDeltaData(data, offset, paletteTable, maxDeltas, snapshotSize));

        return Layer2d{deltas, type};
    }

    void serializeLayers(const LayerContainer2d& layers, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
        const uint64_t layersLength = layers.layers.size();
        data.resize(data.size() + sizeof(uint64_t));
        std::memcpy(data.data() + data.size() - sizeof(uint64_t), &layersLength, sizeof(uint64_t));

        for (const auto& layer: layers.layers | std::views::values)
            serializeLayer(layer, data, paletteTable);
    }

    ZVCRResult<LayerContainer2d> deserializeLayers(const std::vector<uint8_t>& data, size_t& offset,
                                                   const std::vector<Palette>& paletteTable, const size_t maxDeltas, const size_t snapshotSize) {
        if (offset + sizeof(uint64_t) > data.size())
            return Err(EXPECTED_LAYERS_LENGTH);

        uint64_t layersLength;
        std::memcpy(&layersLength, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);

        LayerTable2d layers{snapshotSize};
        for (size_t layerIndex = 0; layerIndex < layersLength; ++layerIndex) {
            const auto layer = Try(deserializeLayer(data, offset, paletteTable, maxDeltas, layers.snapshotSize));
            layers[layer.type] = layer;
        }
        return LayerContainer2d{layers};
    }

    ZVCRResult<LayerContainer2d> deserializeBlockLayers(const std::vector<uint8_t>& data, size_t& offset,
                                                        const std::vector<Palette>& paletteTable, const size_t maxDeltas) {
        return deserializeLayers(data, offset, paletteTable, maxDeltas, SECTION_2D_SIZE_BLOCKS);
    }

    ZVCRResult<LayerContainer2d> deserializeBiomeLayers(const std::vector<uint8_t>& data, size_t& offset,
                                                        const std::vector<Palette>& paletteTable, const size_t maxDeltas,
                                                        const ZVCR2Version version) {
        if (version < ZVCR2Version::ZVCR2_0_1_0_0)
            return LayerContainer2d{SECTION_2D_SIZE_BIOMES};

        return deserializeLayers(data, offset, paletteTable, maxDeltas, SECTION_2D_SIZE_BIOMES);
    }

    void serializeSegment2d(const Segment2d& segment, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable,
                            const ZVCR2Version version) {
        serializeLayers(segment.layers, data, paletteTable);

        if (version >= ZVCR2Version::ZVCR2_0_1_0_0)
            serializeLayers(segment.biomeLayers, data, paletteTable);

        serializeSegmentInfo(segment.info, data);
    }

    ZVCRResult<Segment2d> deserializeSegment2d(const std::vector<uint8_t>& data, size_t& offset,
                                               const std::vector<Palette>& paletteTable, const size_t maxDeltas,
                                               const ZVCR2Version version) {
        return Segment2d {
            Try(deserializeBlockLayers(data, offset, paletteTable, maxDeltas)),
            Try(deserializeBiomeLayers(data, offset, paletteTable, maxDeltas, version)),
            Try(deserializeSegmentInfo(data, offset))
        };
    }

    void serializeOptSegment2d(const Option<Segment2d>& segment, std::vector<uint8_t>& data,
                               std::vector<Palette>& paletteTable, const ZVCR2Version version) {
        if (segment.none()) {
            data.push_back(0);
            return;
        }
        data.push_back(1);
        serializeSegment2d(segment.unwrap(), data, paletteTable, version);
    }

    ZVCRResult<Option<Segment2d>> deserializeOptSegment2d(const std::vector<uint8_t>& data, size_t& offset,
                                                          const std::vector<Palette>& paletteTable, const size_t maxDeltas,
                                                          const ZVCR2Version version) {
        if (offset >= data.size())
            return Err(EXPECTED_SEGMENT_INDICATOR);

        if (data[offset++] == 0)
            return Option<Segment2d>();

        return static_cast<ZVCRResult<Option<Segment2d>>>(
            Try(deserializeSegment2d(data, offset, paletteTable, maxDeltas, version))
        );
    }

    void serializeRegion2d(const Region2d& region, std::vector<uint8_t>& data, const ZVCR2Version version) {
        std::vector<Palette> paletteTable;
        std::vector<uint8_t> regionData;

        for (const auto& segment : region.segments)
            serializeOptSegment2d(segment, regionData, paletteTable, version);

        serializePaletteTable(paletteTable, data);
        data.insert(data.end(), regionData.begin(), regionData.end());
    }

    ZVCRResult<Region2d> deserializeRegion2d(const std::vector<uint8_t>& data, size_t& offset, const size_t maxDeltas, const ZVCR2Version version) {
        const auto paletteTable = Try(deserializePaletteTable(data, offset));
        Segments2d segment3ds(SEGMENTS_PER_REGION);
        for (size_t segmentIndex = 0; segmentIndex < SEGMENTS_PER_REGION; ++segmentIndex)
            segment3ds[segmentIndex] = Try(deserializeOptSegment2d(data, offset, paletteTable, maxDeltas, version));

        return Region2d{segment3ds};
    }

    void serializeZVCR2File(const ZVCR2File& file, std::vector<uint8_t>& data) {
        const std::string prefix = ZVCR2_FILE_PREFIX;
        data.insert(data.end(), prefix.begin(), prefix.end());
        data.push_back(static_cast<uint8_t>(file.version));
        data.push_back(static_cast<uint8_t>(file.dimensionType));
        serializeRegion2d(file.region, data, file.version);
    }

    ZVCRResult<ZVCR2File> deserializeZVCR2File(const std::vector<uint8_t>& data, size_t& offset, const size_t maxDeltas) {
        validateZVCRFilePrefix(data, offset, ZVCR2_FILE_PREFIX);
        const auto version = Try(deserializeVersion(data, offset, ZVCR2_VER_LATEST));
        const auto dimensionType = Try(deserializeDimensionType(data, offset));
        const auto region = Try(deserializeRegion2d(data, offset, maxDeltas, version));

        return ZVCR2File{version, dimensionType, region};
    }

}
