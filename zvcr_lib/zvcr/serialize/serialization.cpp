#include <zvcr/serialize/serialization.hpp>
#include <fstream>
#include <filesystem>
#include <ranges>
#include <cstring>
#include <zvcr/common/definitions.hpp>

namespace zvcr {

    std::string ReadError::what() const {
        std::ostringstream oss;
        oss << "Read error code " << std::hex << type << std::dec << ": " << message;

        if (offset > 0)
            oss << " at read offset = " << offset;

        if (!dumpSlice.empty()) {
            oss << " :: Debug: [ ";
            if (sliceStart > 0) oss << "... ";

            for (size_t i = 0; i < dumpSlice.size(); i++) {
                const auto byte = dumpSlice[i];
                if (i == dumpPoint) oss << "| ";

                oss << std::uppercase << std::setfill('0') << std::setw(2) << std::hex;
                oss << static_cast<int>(byte);

                oss << ' ';
            }
            if (sliceEnd < dataLength) oss << "... ";
            oss << ']';
        }
        return oss.str();
    }

    void ReadError::attach(const ReadHandle& handle) {
        if (handle.data.empty()) return;

        dataLength = handle.data.size();
        sliceStart = offset >= DBG_SLICE_SIZE ? offset - DBG_SLICE_SIZE : 0;
        sliceEnd = std::min(offset + DBG_SLICE_SIZE + 1, dataLength);
        dumpPoint = offset - sliceStart;
        dumpSlice = std::vector(handle.data.begin() + static_cast<int64_t>(sliceStart), handle.data.begin() + static_cast<int64_t>(sliceEnd));
    }

    ReadResult<DimensionType> ReadHandle::deserializeDimensionType() {
        const auto dimensionTypeId = Try(readByte(EXPECTED_DIMENSION_TYPE));
        static constexpr auto maxDimensionTypeId = static_cast<uint8_t>(DimensionType::THE_END);

        if (dimensionTypeId > maxDimensionTypeId) {
            const auto err = ReadError{INVALID_DIMENSION_TYPE, offset, "Invalid dimension type: "
                + std::to_string(dimensionTypeId) + " > " + std::to_string(maxDimensionTypeId)};

            return Err(err);
        }
        const auto dimensionType = static_cast<DimensionType>(dimensionTypeId);
        sectionCount = getProperties(dimensionType).height / SEGMENT_SIDELENGTH_BLOCKS;

        return dimensionType;
    }

    ReadResult<std::monostate> ReadHandle::validateZVCRFilePrefix(const std::string& prefix) {
        Try(skip<uint8_t>(prefix.size(), MISSING_HEADER));

        for (size_t i = 0; i < prefix.size(); ++i) {
            if (const auto expected = static_cast<uint8_t>(prefix[i]); data[i] != expected)
                return Err(ReadError(INVALID_HEADER_PREFIX, offset, "Invalid header prefix: "
                    + std::to_string(data[i]) + " != " + std::to_string(expected)));
        }
        return {};
    }

    void WriteHandle::serializePackedSnapshot(const PackedSnapshot<SegmentAtom>& snapshot) {
        write<uint64_t>(snapshot.timestamp);

        const auto& packedData = snapshot.data.packedData;
        const auto packedLength = packedData.size();

        write<uint64_t>(packedLength);
        writeArray(packedData.data(), packedLength);

        const auto paletteTableLength = paletteTableStorage.size();
        const auto& palette = snapshot.data.palette;
        size_t paletteIndex = paletteTableLength;
        for (size_t i = 0; i < paletteTableLength; ++i) {
            if (const auto& existingPalette = paletteTableStorage[i]; palette == existingPalette) {
                paletteIndex = i;
                break;
            }
        }
        if (paletteIndex == paletteTableLength)
            paletteTableStorage.push_back(palette);

        write<uint32_t>(paletteIndex);
    }

    ReadResult<PackedSnapshot<SegmentAtom>> ReadHandle::deserializePackedSnapshot(const size_t snapshotLength) {
        const auto timestamp = static_cast<time_t>(Try(read<uint64_t>(EXPECTED_TIMESTAMP)));
        const auto packedLength = Try(read<uint64_t>(EXPECTED_PACKED_LENGTH));
        if (packedLength > MAX_PACKED_LENGTH) {
            const auto err = ReadError{INVALID_PACKED_LENGTH, offset, "Invalid packed length: "
                + std::to_string(packedLength) + " > " + std::to_string(MAX_PACKED_LENGTH)};
            return Err(err);
        }
        LongArray packedData(packedLength);
        Try(readArray(packedData, EXPECTED_PACKED_DATA));

        const auto paletteIndex = Try(read<uint32_t>(EXPECTED_PALETTE_INDEX));
        const auto& palette = paletteTable[paletteIndex];

        if (paletteIndex >= paletteTable.size()) {
            const auto err = ReadError{INVALID_PALETTE_INDEX, offset, "Invalid palette index: "
                + std::to_string(paletteIndex) + " >= " + std::to_string(paletteTable.size())};
            return Err(err);
        }
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

    ReadResult<std::monostate> ReadHandle::deserializePaletteTable() {
        const auto paletteTableLength = Try(read<uint32_t>(EXPECTED_PALETTE_TABLE_LENGTH));
        if (paletteTableLength > MAX_PALETTE_TABLE_LENGTH)
            return Err(ReadError(INVALID_PALETTE_TABLE_LENGTH, offset, "Invalid palette table length: "
                + std::to_string(paletteTableLength) + " > " + std::to_string(MAX_PALETTE_TABLE_LENGTH)));

        paletteTable.reserve(paletteTableLength);

        for (size_t i = 0; i < paletteTableLength; ++i) {
            const auto paletteLength = static_cast<size_t>(Try(read<uint16_t>(EXPECTED_PALETTE_LENGTH)));

            Palette palette(paletteLength);
            Try(readArray(palette, EXPECTED_PALETTE_DATA));
            paletteTable.push_back(palette);
        }
        return {};
    }

    ReadResult<std::monostate> ReadHandle::skipPackedSnapshot() {
        Try(skip<uint64_t>(EXPECTED_TIMESTAMP));
        const auto packedLength = Try(read<uint64_t>(EXPECTED_PACKED_LENGTH));

        Try(skip<uint64_t>(packedLength, EXPECTED_PACKED_DATA));
        Try(skip<uint32_t>(EXPECTED_PALETTE_INDEX));
        return {};
    }

    void WriteHandle::serializePackedDeltaData(const PackedDeltaData<SegmentAtom>& section3d) {
        write<uint64_t>(section3d.reverseDeltas.size());

        for (const PackedSnapshot<SegmentAtom>& snapshot : section3d.reverseDeltas)
            serializePackedSnapshot(snapshot);
    }

    ReadResult<PackedDeltaData<SegmentAtom>> ReadHandle::deserializePackedDeltaData(const size_t snapshotLength) {
        const auto deltaLength = Try(read<uint64_t>(EXPECTED_DELTA_LENGTH));
        if (deltaLength > MAX_DELTA_LENGTH) {
            const auto err = ReadError{INVALID_DELTA_LENGTH, offset, "Invalid delta length: "
                + std::to_string(deltaLength) + " > " + std::to_string(MAX_DELTA_LENGTH)};
            return Err(err);
        }
        std::vector<PackedSnapshot<SegmentAtom>> reverseDeltas;
        reverseDeltas.reserve(deltaLength);

        for (size_t deltaIndex = 0; deltaIndex < deltaLength; ++deltaIndex) {
            if (maxDeltas != 0 && deltaIndex >= maxDeltas) {
                Try(skipPackedSnapshot());
                continue;
            }
            reverseDeltas.push_back(Try(deserializePackedSnapshot(snapshotLength)));
        }
        return PackedDeltaData{reverseDeltas, snapshotLength};
    }

    void WriteHandle::serializeSegmentState(const SegmentState& segmentState) {
        writeByte(static_cast<uint8_t>(segmentState.type));
        write<uint64_t>(segmentState.timestamp);
    }

    ReadResult<SegmentState> ReadHandle::deserializeSegmentState() {
        const auto stateTypeId = Try(readByte(EXPECTED_SEGMENT_STATE_TYPE));
        const auto stateType = static_cast<SegmentStateType>(stateTypeId);
        const auto timestamp = static_cast<time_t>(Try(read<uint64_t>(EXPECTED_SEGMENT_STATE_TIMESTAMP)));

        return SegmentState{stateType, timestamp};
    }

    void WriteHandle::serializeTileEntityCountInfo(const TileEntityCountInfo& tileEntityCounts) {
        const auto length = tileEntityCounts.counts.size();
        assert(length == getTotalTileEntities(ctx.protocolVersion) && "Attempting to write unconverted tileEntityCounts");

        writeArray(tileEntityCounts.counts.data(), length);
        write<uint64_t>(tileEntityCounts.timestamp);
    }

    ReadResult<TileEntityCountInfo> ReadHandle::deserializeTileEntityCountInfo() {
        const auto totalTileEntities = getTotalTileEntities(ctx.protocolVersion);

        std::vector<TileEntityType> counts(totalTileEntities);
        Try(readArray(counts, EXPECTED_TILE_ENTITY_COUNTS));
        const auto timestamp = static_cast<time_t>(Try(read<uint64_t>(EXPECTED_TILE_ENTITY_COUNTS_TIMESTAMP)));

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
        if (statesLength > MAX_SEGMENT_STATES_LENGTH) {
            const auto err = ReadError{INVALID_SEGMENT_STATES_LENGTH, offset, "Invalid segment states length: "
                + std::to_string(statesLength) + " > " + std::to_string(MAX_SEGMENT_STATES_LENGTH)};
            return Err(err);
        }

        SegmentStates states;
        states.reserve(statesLength);

        for (size_t i = 0; i < statesLength; ++i)
            states.push_back(Try(deserializeSegmentState()));

        const auto tileEntitiesLength = Try(read<uint64_t>(EXPECTED_TILE_ENTITIES_LENGTH));
        if (tileEntitiesLength > MAX_TILE_ENTITIES_LENGTH) {
            const auto err = ReadError{INVALID_TILE_ENTITIES_LENGTH, offset, "Invalid tile entities length: "
                + std::to_string(tileEntitiesLength) + " > " + std::to_string(MAX_TILE_ENTITIES_LENGTH)};
            return Err(err);
        }

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
        if (!segment3dOpt.has_value()) {
            writeByte(0);
            return;
        }
        writeByte(1);
        serializeSegment3d(segment3dOpt.value());
    }

    ReadResult<Option<Segment3d>> ReadHandle::deserializeOptSegment3d() {
        if (const bool hasOption = Try(readByte(EXPECTED_SEGMENT_INDICATOR)); !hasOption)
            return None;

        return Try(deserializeSegment3d());
    }

    void WriteHandle::serializeRegion3d(const Region3d& region) {
        WriteHandle regionDataHandle{};
        regionDataHandle.ctx = ctx;

        for (const auto& segment3d : region.segments)
            regionDataHandle.serializeOptSegment3d(segment3d);

        if (ctx.supportDynamicVersioning)
            write<uint16_t>(ctx.protocolVersion);

        serializePaletteTable(regionDataHandle.paletteTableStorage);
        writeBytes(regionDataHandle.data);
    }

    ReadResult<Region3d> ReadHandle::deserializeRegion3d(const ZVCR3Version version) {
        ctx.initialize(version);

        if (ctx.supportDynamicVersioning)
            ctx.protocolVersion = Try(read<uint16_t>(EXPECTED_PROTOCOL_VERSION));

        Try(deserializePaletteTable());
        Segments3d segment3ds(SEGMENTS_PER_REGION);
        for (size_t segment3dIndex = 0; segment3dIndex < SEGMENTS_PER_REGION; ++segment3dIndex)
            segment3ds[segment3dIndex] = Try(deserializeOptSegment3d());

        return Region3d{segment3ds, ctx.protocolVersion};
    }

    void serializeZVCR3File(const ZVCR3File& file, WriteHandle& handle) {
        const std::string prefix = ZVCR3_FILE_PREFIX;

        handle.writeBytes(prefix);
        handle.writeByte(static_cast<uint8_t>(file.version));
        handle.writeByte(static_cast<uint8_t>(file.dimensionType));

        handle.ctx.initialize(file.version);
        handle.serializeRegion3d(file.region);
    }

    ReadResult<ZVCR3File> deserializeZVCR3File(ReadHandle& handle) {
        Try(handle.validateZVCRFilePrefix(ZVCR3_FILE_PREFIX));

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
            Try(deserializeSegmentInfo()),
            ctx.supportBiomes
        };
    }

    void WriteHandle::serializeOptSegment2d(const Option<Segment2d>& segment) {
        if (!segment.has_value()) {
            writeByte(0);
            return;
        }
        writeByte(1);
        serializeSegment2d(segment.value());
    }

    ReadResult<Option<Segment2d>> ReadHandle::deserializeOptSegment2d() {
        if (const auto hasOption = Try(readByte(EXPECTED_SEGMENT_INDICATOR)); !hasOption)
            return None;

        return Try(deserializeSegment2d());
    }

    void WriteHandle::serializeRegion2d(const Region2d& region) {
        WriteHandle regionDataHandle{};
        regionDataHandle.ctx = ctx;

        for (const auto& segment : region.segments)
            regionDataHandle.serializeOptSegment2d(segment);

        if (ctx.supportDynamicVersioning)
            write<uint16_t>(ctx.protocolVersion);

        serializePaletteTable(regionDataHandle.paletteTableStorage);
        writeBytes(regionDataHandle.data);
    }

    ReadResult<Region2d> ReadHandle::deserializeRegion2d(const ZVCR2Version version) {
        ctx.initialize(version);

        if (ctx.supportDynamicVersioning)
            ctx.protocolVersion = Try(read<uint16_t>(EXPECTED_PROTOCOL_VERSION));

        Try(deserializePaletteTable());
        Segments2d segment3ds(SEGMENTS_PER_REGION);
        for (size_t segmentIndex = 0; segmentIndex < SEGMENTS_PER_REGION; ++segmentIndex)
            segment3ds[segmentIndex] = Try(deserializeOptSegment2d());

        return Region2d{segment3ds, ctx.protocolVersion};
    }

    void serializeZVCR2File(const ZVCR2File& file, WriteHandle& handle) {
        const std::string prefix = ZVCR2_FILE_PREFIX;
        handle.writeBytes(prefix);
        handle.writeByte(static_cast<uint8_t>(file.version));
        handle.writeByte(static_cast<uint8_t>(file.dimensionType));

        handle.ctx.initialize(file.version);
        handle.serializeRegion2d(file.region);
    }

    ReadResult<ZVCR2File> deserializeZVCR2File(ReadHandle& handle) {
        Try(handle.validateZVCRFilePrefix(ZVCR2_FILE_PREFIX));

        const auto version = Try(handle.deserializeVersion(ZVCR2_VER_LATEST));
        const auto dimensionType = Try(handle.deserializeDimensionType());
        const auto region = Try(handle.deserializeRegion2d(version));

        return ZVCR2File{version, dimensionType, region};
    }

}
