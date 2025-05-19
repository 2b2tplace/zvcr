#include <zvcr/serialize/serialization.hpp>
#include <fstream>
#include <filesystem>
#include <ranges>
#include <cstring>
#include <zvcr/common/definitions.hpp>
#include <zvcr/serialize/compression.hpp>

namespace zvcr::serialize {

    ReadResult<DimensionType> ReadHandle::deserializeDimensionType() {
        const auto dimensionTypeId = Try(readByte(EXPECTED_DIMENSION_TYPE));

        if (dimensionTypeId > static_cast<uint8_t>(DimensionType::THE_END))
            return Err(INVALID_DIMENSION_TYPE);

        const auto dimensionType = static_cast<DimensionType>(dimensionTypeId);
        sectionCount = getProperties(dimensionType).height / SEGMENT_SIDELENGTH_BLOCKS;

        return dimensionType;
    }

    Option<ReadError> ReadHandle::validateZVCRFilePrefix(const std::string& prefix) {
        skip<uint8_t>(prefix.size(), MISSING_HEADER);

        for (size_t i = 0; i < prefix.size(); ++i) {
            if (data[i] != static_cast<uint8_t>(prefix[i]))
                return INVALID_HEADER_PREFIX;
        }
        return {};
    }

    void WriteHandle::serializePackedSnapshot(const PackedSnapshot<SegmentAtom>& snapshot) {
        write(snapshot.timestamp);

        const auto& packedData = snapshot.data.packedData;
        const auto packedLength = packedData.size();

        write<uint64_t>(packedLength);
        writeArray(packedData.data(), packedLength);

        const auto paletteTableLength = paletteTable.size();
        const auto& palette = snapshot.data.palette;
        size_t paletteIndex = paletteTableLength;
        for (size_t i = 0; i < paletteTableLength; ++i) {
            if (const auto& existingPalette = paletteTable[i]; palette == existingPalette) {
                paletteIndex = i;
                break;
            }
        }
        if (paletteIndex == paletteTableLength)
            paletteTable.push_back(palette);

        write(paletteIndex);
    }

    ReadResult<PackedSnapshot<SegmentAtom>> ReadHandle::deserializePackedSnapshot(const size_t snapshotLength) {
        const auto timestamp = Try(read<time_t>(EXPECTED_TIMESTAMP));
        const auto packedLength = Try(read<uint64_t>(EXPECTED_PACKED_LENGTH));

        LongArray packedData(packedLength);
        Propagate(readArray(packedData, EXPECTED_PACKED_DATA));

        const auto paletteIndex = Try(read<uint32_t>(EXPECTED_PALETTE_INDEX));
        const auto& palette = paletteTable[paletteIndex];

        if (paletteIndex >= paletteTable.size())
            return Err(INVALID_PALETTE_INDEX);

        return PackedSnapshot {
            PackedData{palette, packedData, snapshotLength},
            timestamp
        };
    }

    void WriteHandle::serializePaletteTable(const std::vector<Palette>& paletteTable) {
        write(static_cast<uint32_t>(paletteTable.size()));

        for (const auto& palette : paletteTable) {
            const auto paletteLength = palette.size();

            write<uint16_t>(paletteLength);
            writeArray(palette.data(), paletteLength);
        }
    }

    Option<ReadError> ReadHandle::deserializePaletteTable() {
        const auto paletteTableLength = Require(read<uint32_t>(EXPECTED_PALETTE_TABLE_LENGTH));
        paletteTable.reserve(paletteTableLength);

        for (size_t i = 0; i < paletteTableLength; ++i) {
            const auto paletteLength = static_cast<size_t>(Require(read<uint16_t>(EXPECTED_PALETTE_LENGTH)));

            Palette palette(paletteLength);
            readArray(palette, EXPECTED_PALETTE_DATA);
            paletteTable.emplace_back(palette);
        }
        return {};
    }

    Option<ReadError> ReadHandle::skipPackedSnapshot() {
        skip<time_t>(EXPECTED_TIMESTAMP);
        const auto packedLength = Require(read<uint64_t>(EXPECTED_PACKED_LENGTH));

        skip<uint64_t>(packedLength, EXPECTED_PACKED_DATA);
        skip<uint32_t>(EXPECTED_PALETTE_INDEX);
        return {};
    }

    void WriteHandle::serializePackedDeltaData(const PackedDeltaData<SegmentAtom>& section3d) {
        write<uint64_t>(section3d.reverseDeltas.size());

        for (const PackedSnapshot<SegmentAtom>& snapshot : section3d.reverseDeltas)
            serializePackedSnapshot(snapshot);
    }

    ReadResult<PackedDeltaData<SegmentAtom>> ReadHandle::deserializePackedDeltaData(const size_t snapshotLength) {
        const auto deltaLength = Try(read<uint64_t>(EXPECTED_DELTA_LENGTH));

        std::vector<PackedSnapshot<SegmentAtom>> reverseDeltas;
        reverseDeltas.reserve(deltaLength);

        for (size_t deltaIndex = 0; deltaIndex < deltaLength; ++deltaIndex) {
            if (maxDeltas != 0 && deltaIndex >= maxDeltas) {
                Propagate(skipPackedSnapshot());
                continue;
            }
            reverseDeltas.push_back(Try(deserializePackedSnapshot(snapshotLength)));
        }
        return PackedDeltaData{reverseDeltas, snapshotLength};
    }

    void WriteHandle::serializeSegmentState(const SegmentState& segmentState) {
        writeByte(static_cast<uint8_t>(segmentState.type));
        write<time_t>(segmentState.timestamp);
    }

    ReadResult<SegmentState> ReadHandle::deserializeSegmentState() {
        const auto stateTypeId = Try(readByte(EXPECTED_SEGMENT_STATE_TYPE));
        const auto stateType = static_cast<SegmentStateType>(stateTypeId);
        const auto timestamp = Try(read<time_t>(EXPECTED_SEGMENT_STATE_TIMESTAMP));

        return SegmentState{stateType, timestamp};
    }

    void WriteHandle::serializeTileEntityCountInfo(const TileEntityCountInfo& tileEntityCounts) {
        const auto length = tileEntityCounts.counts.size();
        assert(length == getTotalTileEntities(ctx.protocolVersion) && "Attempting to write unconverted tileEntityCounts");

        writeArray(tileEntityCounts.counts.data(), length);
        write(tileEntityCounts.timestamp);
    }

    ReadResult<TileEntityCountInfo> ReadHandle::deserializeTileEntityCountInfo() {
        const auto totalTileEntities = getTotalTileEntities(ctx.protocolVersion);

        std::vector<uint16_t> counts(totalTileEntities);
        readArray(counts, EXPECTED_TILE_ENTITY_COUNTS);
        const auto timestamp = Try(read<time_t>(EXPECTED_TILE_ENTITY_COUNTS_TIMESTAMP));

        return TileEntityCountInfo{counts, timestamp};
    }

    void WriteHandle::serializeSegmentInfo(const SegmentInfo& segmentInfo) {
        write<uint64_t>(segmentInfo.segmentStates.size());

        for (const SegmentState& state : segmentInfo.segmentStates)
            serializeSegmentState(state);

        write<uint64_t>(segmentInfo.tileEntityCounts.size());

        for (const TileEntityCountInfo& tiles : segmentInfo.tileEntityCounts)
            serializeTileEntityCountInfo(tiles);
    }

    ReadResult<SegmentInfo> ReadHandle::deserializeSegmentInfo() {
        const auto statesLength = Try(read<uint64_t>(EXPECTED_SEGMENT_STATES_LENGTH));

        SegmentStates states;
        states.reserve(statesLength);

        for (size_t i = 0; i < statesLength; ++i)
            states.push_back(Try(deserializeSegmentState()));

        const auto tileEntitiesLength = Try(read<uint64_t>(EXPECTED_TILE_ENTITIES_LENGTH));

        TileEntityCounts tileEntityCounts;
        tileEntityCounts.reserve(tileEntitiesLength);

        for (size_t i = 0; i < tileEntitiesLength; ++i)
            tileEntityCounts.push_back(Try(deserializeTileEntityCountInfo()));

        return SegmentInfo{states, tileEntityCounts};
    }

    void WriteHandle::serializeSegment3d(const Segment3d& segment3d) {
        for (const PackedDeltaData<SegmentAtom>& section : segment3d.blockSections.getSections())
            serializePackedDeltaData(section);

        if (ctx.supportBiomes) {
            for (const PackedDeltaData<SegmentAtom>& section : segment3d.biomeSections.getSections())
                serializePackedDeltaData(section);
        }
        serializeSegmentInfo(segment3d.info);
    }

    ReadResult<Segment3d> ReadHandle::deserializeSegment3d() {
        Segment3d segment{sectionCount, ctx.supportBiomes};

        for (size_t sectionIndex = 0; sectionIndex < sectionCount; ++sectionIndex)
            segment.blockSections.getSection(sectionIndex) = Try(deserializePackedDeltaData(SECTION_3D_SIZE_BLOCKS));

        if (ctx.supportBiomes) {
            for (size_t sectionIndex = 0; sectionIndex < sectionCount; ++sectionIndex)
                segment.biomeSections.getSection(sectionIndex) = Try(deserializePackedDeltaData(SECTION_3D_SIZE_BIOMES));
        }
        segment.info = Try(deserializeSegmentInfo());
        return segment;
    }

    void WriteHandle::serializeOptSegment3d(const Option<Segment3d>& segment3dOpt) {
        if (segment3dOpt.none()) {
            writeByte(0);
            return;
        }
        writeByte(1);
        serializeSegment3d(segment3dOpt.unwrap());
    }

    ReadResult<Option<Segment3d>> ReadHandle::deserializeOptSegment3d() {
        if (const bool hasOption = Try(readByte(EXPECTED_SEGMENT_INDICATOR)); !hasOption)
            return Option<Segment3d>();

        return static_cast<ReadResult<Option<Segment3d>>>(Try(deserializeSegment3d()));
    }

    void WriteHandle::serializeRegion3d(const Region3d& region, const ZVCR3Version version) {
        WriteHandle regionDataHandle{};
        regionDataHandle.ctx.initialize(version);

        if (ctx.supportDynamicVersioning)
            regionDataHandle.ctx.protocolVersion = PROTOCOL_VERSION;

        for (const auto& segment3d : region.segments)
            regionDataHandle.serializeOptSegment3d(segment3d);

        if (ctx.supportDynamicVersioning)
            write<uint16_t>(ctx.protocolVersion);

        serializePaletteTable(regionDataHandle.paletteTable);
        writeBytes(regionDataHandle.data);
    }

    ReadResult<Region3d> ReadHandle::deserializeRegion3d(const ZVCR3Version version) {
        ctx.initialize(version);

        if (ctx.supportDynamicVersioning)
            ctx.protocolVersion = Try(read<uint16_t>(EXPECTED_PROTOCOL_VERSION));

        Propagate(deserializePaletteTable());
        Segments3d segment3ds(SEGMENTS_PER_REGION);
        for (size_t segment3dIndex = 0; segment3dIndex < SEGMENTS_PER_REGION; ++segment3dIndex)
            segment3ds[segment3dIndex] = Try(deserializeOptSegment3d());

        return Region3d{segment3ds};
    }

    void serializeZVCR3File(const ZVCR3File& file, WriteHandle& handle) {
        const std::string prefix = ZVCR3_FILE_PREFIX;

        handle.writeBytes(prefix);
        handle.writeByte(static_cast<uint8_t>(file.version));
        handle.writeByte(static_cast<uint8_t>(file.dimensionType));
        handle.serializeRegion3d(file.region, file.version);
    }

    ReadResult<ZVCR3File> deserializeZVCR3File(ReadHandle& handle) {
        handle.validateZVCRFilePrefix(ZVCR3_FILE_PREFIX);

        const auto version = Try(handle.deserializeVersion(ZVCR3_VER_LATEST));
        const auto dimensionType = Try(handle.deserializeDimensionType());
        const auto region = Try(handle.deserializeRegion3d(version));

        return ZVCR3File{version, dimensionType, region};
    }

    void WriteHandle::serializeLayer(const Layer2d& layer) {
        writeByte(layer.type);
        serializePackedDeltaData(layer.deltas);
    }

    ReadResult<Layer2d> ReadHandle::deserializeLayer(const size_t snapshotSize) {
        const auto type = Try(readByte(EXPECTED_LAYER_TYPE));
        const auto deltas = Try(deserializePackedDeltaData(snapshotSize));

        return Layer2d{deltas, type};
    }

    void WriteHandle::serializeLayers(const LayerContainer2d& layers) {
        write<uint64_t>(layers.layers.size());

        for (const auto& layer: layers.layers | std::views::values)
            serializeLayer(layer);
    }

    ReadResult<LayerContainer2d> ReadHandle::deserializeLayers(const size_t snapshotSize) {
        const auto layersLength = Try(read<uint64_t>(EXPECTED_LAYERS_LENGTH));

        LayerTable2d layers{snapshotSize};
        for (size_t layerIndex = 0; layerIndex < layersLength; ++layerIndex) {
            const auto layer = Try(deserializeLayer(snapshotSize));
            layers[layer.type] = layer;
        }
        return LayerContainer2d{layers};
    }

    ReadResult<LayerContainer2d> ReadHandle::deserializeBlockLayers() {
        return deserializeLayers(SECTION_2D_SIZE_BLOCKS);
    }

    ReadResult<LayerContainer2d> ReadHandle::deserializeBiomeLayers() {
        if (ctx.supportBiomes)
            return deserializeLayers(SECTION_2D_SIZE_BIOMES);

        return LayerContainer2d{SECTION_2D_SIZE_BIOMES};
    }

    void WriteHandle::serializeSegment2d(const Segment2d& segment) {
        serializeLayers(segment.layers);

        if (ctx.supportBiomes)
            serializeLayers(segment.biomeLayers);

        serializeSegmentInfo(segment.info);
    }

    ReadResult<Segment2d> ReadHandle::deserializeSegment2d() {
        return Segment2d {
            Try(deserializeBlockLayers()),
            Try(deserializeBiomeLayers()),
            Try(deserializeSegmentInfo())
        };
    }

    void WriteHandle::serializeOptSegment2d(const Option<Segment2d>& segment) {
        if (segment.none()) {
            writeByte(0);
            return;
        }
        writeByte(1);
        serializeSegment2d(segment.unwrap());
    }

    ReadResult<Option<Segment2d>> ReadHandle::deserializeOptSegment2d() {
        if (const auto hasOption = Try(readByte(EXPECTED_SEGMENT_INDICATOR)); !hasOption)
            return Option<Segment2d>{};

        return static_cast<ReadResult<Option<Segment2d>>>(Try(deserializeSegment2d()));
    }

    void WriteHandle::serializeRegion2d(const Region2d& region, const ZVCR2Version version) {
        WriteHandle regionDataHandle{};
        regionDataHandle.ctx.initialize(version);

        for (const auto& segment : region.segments)
            regionDataHandle.serializeOptSegment2d(segment);

        serializePaletteTable(regionDataHandle.paletteTable);
        writeBytes(regionDataHandle.data);
    }

    ReadResult<Region2d> ReadHandle::deserializeRegion2d(const ZVCR2Version version) {
        ctx.initialize(version);

        Propagate(deserializePaletteTable());
        Segments2d segment3ds(SEGMENTS_PER_REGION);
        for (size_t segmentIndex = 0; segmentIndex < SEGMENTS_PER_REGION; ++segmentIndex)
            segment3ds[segmentIndex] = Try(deserializeOptSegment2d());

        return Region2d{segment3ds};
    }

    void serializeZVCR2File(const ZVCR2File& file, WriteHandle& handle) {
        const std::string prefix = ZVCR2_FILE_PREFIX;
        handle.writeBytes(prefix);
        handle.writeByte(static_cast<uint8_t>(file.version));
        handle.writeByte(static_cast<uint8_t>(file.dimensionType));
        handle.serializeRegion2d(file.region, file.version);
    }

    ReadResult<ZVCR2File> deserializeZVCR2File(ReadHandle& handle) {
        handle.validateZVCRFilePrefix(ZVCR2_FILE_PREFIX);

        const auto version = Try(handle.deserializeVersion(ZVCR2_VER_LATEST));
        const auto dimensionType = Try(handle.deserializeDimensionType());
        const auto region = Try(handle.deserializeRegion2d(version));

        return ZVCR2File{version, dimensionType, region};
    }

}
