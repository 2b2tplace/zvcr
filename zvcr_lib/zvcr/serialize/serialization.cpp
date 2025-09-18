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
        const auto dimensionTypeId = TRY(readByte(EXPECTED_DIMENSION_TYPE));
        static constexpr auto maxDimensionTypeId = static_cast<uint8_t>(DimensionType::THE_END);

        if (dimensionTypeId > maxDimensionTypeId) {
            const auto err = ReadError{INVALID_DIMENSION_TYPE, offset, "Invalid dimension type: "
                + std::to_string(dimensionTypeId) + " > " + std::to_string(maxDimensionTypeId)};

            return ERR(err);
        }
        const auto dimensionType = static_cast<DimensionType>(dimensionTypeId);
        sectionCount = getProperties(dimensionType).height / SEGMENT_SIDELENGTH_BLOCKS;

        return dimensionType;
    }

    ReadResult<std::monostate> ReadHandle::validateZVCRFilePrefix(const std::string& prefix) {
        TRY(skip<uint8_t>(prefix.size(), MISSING_HEADER));

        for (size_t i = 0; i < prefix.size(); ++i) {
            if (const auto expected = static_cast<uint8_t>(prefix[i]); data[i] != expected)
                return ERR(ReadError(INVALID_HEADER_PREFIX, offset, "Invalid header prefix: "
                    + std::to_string(data[i]) + " != " + std::to_string(expected)));
        }
        return {};
    }

    template<size_t snapshotLength>
    void WriteHandle::serializePackedSnapshot(const PackedSnapshot<snapshotLength>& snapshot) {
        write<uint64_t>(snapshot.timestamp);

        const auto& packedData = snapshot.data.bitStorage.data;
        const auto packedLength = snapshot.data.bitStorage.packedLength;

        write<uint64_t>(packedLength);
        writeArray(packedData.data(), packedLength);

        const auto paletteTableLength = paletteTableStorage.size();
        const auto& palette = snapshot.data.palette;
        if (palette.direct()) {
            // direct palette update; don't store direct palettes, use uint32 max to encode direct palette
            write<uint32_t>(UINT32_MAX);
            return;
        }
        size_t paletteIndex = paletteTableLength;
        for (size_t i = 0; i < paletteTableLength; ++i) {
            if (const auto& existingPalette = paletteTableStorage[i]; palette.equals(existingPalette)) {
                paletteIndex = i;
                break;
            }
        }
        if (paletteIndex == paletteTableLength)
            paletteTableStorage.push_back(palette);

        write<uint32_t>(paletteIndex);
    }

    template<size_t snapshotLength>
    ReadResult<std::monostate> ReadHandle::deserializePackedSnapshot(PackedSnapshot<snapshotLength> &snapshot) {
        snapshot.timestamp = static_cast<time_t>(TRY(read<uint64_t>(EXPECTED_TIMESTAMP)));
        const auto packedLength = TRY(read<uint64_t>(EXPECTED_PACKED_LENGTH));
        if (packedLength > MAX_PACKED_LENGTH) {
            const auto err = ReadError{INVALID_PACKED_LENGTH, offset, "Invalid packed length: "
                + std::to_string(packedLength) + " > " + std::to_string(MAX_PACKED_LENGTH)};
            return ERR(err);
        }
        TRY(readArray(snapshot.data.bitStorage.data, packedLength, EXPECTED_PACKED_DATA));
        snapshot.data.bitStorage.size = snapshotLength;

        const auto paletteIndex = TRY(read<uint32_t>(EXPECTED_PALETTE_INDEX));
        if (paletteIndex == UINT32_MAX) {
            // direct palette update; use uint32 max to encode direct palette
            snapshot.data.palette = DIRECT_PALETTE;
            snapshot.data.bitStorage.bits = snapshot.data.palette.bitsPerIndex;
            snapshot.data.bitStorage.init();
            return {};
        }
        if (paletteIndex >= paletteTable.size()) {
            const auto err = ReadError{INVALID_PALETTE_INDEX, offset, "Invalid palette index: "
                + std::to_string(paletteIndex) + " >= " + std::to_string(paletteTable.size())};
            return ERR(err);
        }
        snapshot.data.palette = paletteTable[paletteIndex];
        snapshot.data.bitStorage.bits = snapshot.data.palette.bitsPerIndex;
        snapshot.data.bitStorage.init();
        return {};
    }

    void WriteHandle::serializePaletteTable(const std::vector<Palette>& paletteTable) {
        write(static_cast<uint32_t>(paletteTable.size()));

        for (const auto& palette : paletteTable) {
            if (palette.direct()) continue;  // direct palette update; don't store direct palettes
            const auto paletteLength = palette.length;

            write<uint16_t>(paletteLength);
            writeArray(palette.palette.data(), paletteLength);
        }
    }

    ReadResult<std::monostate> ReadHandle::deserializePaletteTable() {
        const auto paletteTableLength = TRY(read<uint32_t>(EXPECTED_PALETTE_TABLE_LENGTH));
        if (paletteTableLength > MAX_PALETTE_TABLE_LENGTH)
            return ERR(ReadError(INVALID_PALETTE_TABLE_LENGTH, offset, "Invalid palette table length: "
                + std::to_string(paletteTableLength) + " > " + std::to_string(MAX_PALETTE_TABLE_LENGTH)));

        paletteTable.reserve(paletteTableLength);
        for (size_t i = 0; i < paletteTableLength; ++i) {
            const auto paletteLength = static_cast<size_t>(TRY(read<uint16_t>(EXPECTED_PALETTE_LENGTH)));

            std::array<SegmentAtom, MAX_PALETTE_SIZE> palette{};
            TRY(readArray(palette, std::min(MAX_PALETTE_SIZE, paletteLength), EXPECTED_PALETTE_DATA));

            // direct palette update; needed for backwards compat
            if (paletteLength > MAX_PALETTE_SIZE) {
                paletteTable.push_back(DIRECT_PALETTE);
                continue;
            }
            paletteTable.emplace_back(palette, paletteLength, Palette::getBitsPerIndex(paletteLength));
        }
        return {};
    }

    ReadResult<std::monostate> ReadHandle::skipPackedSnapshot() {
        TRY(skip<uint64_t>(EXPECTED_TIMESTAMP));
        const auto packedLength = TRY(read<uint64_t>(EXPECTED_PACKED_LENGTH));

        TRY(skip<uint64_t>(packedLength, EXPECTED_PACKED_DATA));
        TRY(skip<uint32_t>(EXPECTED_PALETTE_INDEX));
        return {};
    }

    template<size_t snapshotLength>
    void WriteHandle::serializePackedDeltaData(const PackedDeltaData<snapshotLength>& section3d) {
        write<uint64_t>(section3d.reverseDeltas.size());

        for (const PackedSnapshot<snapshotLength>& snapshot : section3d.reverseDeltas)
            serializePackedSnapshot(snapshot);
    }

    template<size_t snapshotLength>
    ReadResult<std::monostate> ReadHandle::deserializePackedDeltaData(PackedDeltaData<snapshotLength> &reverseDeltas) {
        const auto deltaLength = TRY(read<uint64_t>(EXPECTED_DELTA_LENGTH));
        if (deltaLength > MAX_DELTA_LENGTH) {
            const auto err = ReadError{INVALID_DELTA_LENGTH, offset, "Invalid delta length: "
                + std::to_string(deltaLength) + " > " + std::to_string(MAX_DELTA_LENGTH)};
            return ERR(err);
        }
        reverseDeltas.reverseDeltas.resize(deltaLength);
        for (size_t deltaIndex = 0; deltaIndex < deltaLength; ++deltaIndex) {
            if (maxDeltas != 0 && deltaIndex >= maxDeltas) {
                TRY(skipPackedSnapshot());
                continue;
            }
            TRY(deserializePackedSnapshot<snapshotLength>(reverseDeltas.reverseDeltas[deltaIndex]));
        }
        return {};
    }

    void WriteHandle::serializeSegmentState(const SegmentState& segmentState) {
        writeByte(static_cast<uint8_t>(segmentState.type));
        write<uint64_t>(segmentState.timestamp);
    }

    ReadResult<SegmentState> ReadHandle::deserializeSegmentState() {
        const auto stateTypeId = TRY(readByte(EXPECTED_SEGMENT_STATE_TYPE));
        const auto stateType = static_cast<SegmentStateType>(stateTypeId);
        const auto timestamp = static_cast<time_t>(TRY(read<uint64_t>(EXPECTED_SEGMENT_STATE_TIMESTAMP)));

        return SegmentState{stateType, timestamp};
    }

    [[nodiscard]]
    static constexpr size_t getTotalTileEntities(const uint16_t protocolVersion) {
        if (protocolVersion >= 768) return 45;
        if (protocolVersion >= 766) return 44;
        if (protocolVersion >= 765) return 41;

        return -1; // versions pre 1.20.4 are not supported
    }

    ReadResult<std::monostate> ReadHandle::deserializeTileEntityCountInfo() {
        const auto totalTileEntities = getTotalTileEntities(ctx.protocolVersion);

        TRY(skip<uint16_t>(totalTileEntities, EXPECTED_TILE_ENTITY_COUNTS));
        TRY(read<uint64_t>(EXPECTED_TILE_ENTITY_COUNTS_TIMESTAMP));

        return {};
    }

    void WriteHandle::serializeSegmentInfo(const SegmentInfo& segmentInfo) {
        write<uint64_t>(segmentInfo.segmentStates.size());

        for (const SegmentState& state : segmentInfo.segmentStates)
            serializeSegmentState(state);

        write<uint64_t>(0);
    }

    ReadResult<SegmentInfo> ReadHandle::deserializeSegmentInfo() {
        const auto statesLength = TRY(read<uint64_t>(EXPECTED_SEGMENT_STATES_LENGTH));
        if (statesLength > MAX_SEGMENT_STATES_LENGTH) {
            const auto err = ReadError{INVALID_SEGMENT_STATES_LENGTH, offset, "Invalid segment states length: "
                + std::to_string(statesLength) + " > " + std::to_string(MAX_SEGMENT_STATES_LENGTH)};
            return ERR(err);
        }

        SegmentStates states;
        states.resize(statesLength);

        for (size_t i = 0; i < statesLength; ++i)
            states[i] = TRY(deserializeSegmentState());

        const auto tileEntitiesLength = TRY(read<uint64_t>(EXPECTED_TILE_ENTITIES_LENGTH));
        if (tileEntitiesLength > MAX_TILE_ENTITIES_LENGTH) {
            const auto err = ReadError{INVALID_TILE_ENTITIES_LENGTH, offset, "Invalid tile entities length: "
                + std::to_string(tileEntitiesLength) + " > " + std::to_string(MAX_TILE_ENTITIES_LENGTH)};
            return ERR(err);
        }
        for (size_t i = 0; i < tileEntitiesLength; ++i)
            TRY(deserializeTileEntityCountInfo());

        return SegmentInfo{states};
    }

    void WriteHandle::serializeSegment3d(const Segment3d& segment3d) {
        for (const PackedDeltaData<SECTION_3D_SIZE_BLOCKS>& section : segment3d.blockSections.sections)
            serializePackedDeltaData(section);

        if (ctx.supportBiomes) {
            for (const PackedDeltaData<SECTION_3D_SIZE_BIOMES>& section : segment3d.biomeSections.sections)
                serializePackedDeltaData(section);
        }
        serializeSegmentInfo(segment3d.info);
    }

    ReadResult<std::shared_ptr<Segment3d>> ReadHandle::deserializeSegment3d() {
        auto segment = std::make_shared<Segment3d>(sectionCount, ctx.supportBiomes);

        for (size_t sectionIndex = 0; sectionIndex < sectionCount; ++sectionIndex)
            TRY(deserializePackedDeltaData<SECTION_3D_SIZE_BLOCKS>(segment->blockSections.sections[sectionIndex]));

        if (ctx.supportBiomes) {
            for (size_t sectionIndex = 0; sectionIndex < sectionCount; ++sectionIndex)
                TRY(deserializePackedDeltaData<SECTION_3D_SIZE_BIOMES>(segment->biomeSections.sections[sectionIndex]));
        }
        segment->info = TRY(deserializeSegmentInfo());
        return segment;
    }

    void WriteHandle::serializeOptSegment3d(const Segment3d *segment3dOpt) {
        if (!segment3dOpt) {
            writeByte(0);
            return;
        }
        writeByte(1);
        serializeSegment3d(*segment3dOpt);
    }

    void WriteHandle::serializeRegion3d(const Region3d& region) {
        WriteHandle regionDataHandle{};
        regionDataHandle.ctx = ctx;

        for (size_t i = 0; i < SEGMENTS_PER_REGION; i++)
            regionDataHandle.serializeOptSegment3d(region.segments[i].get());

        if (ctx.supportDynamicVersioning)
            write<uint16_t>(ctx.protocolVersion);

        serializePaletteTable(regionDataHandle.paletteTableStorage);
        writeBytes(regionDataHandle.data);
    }

    ReadResult<std::monostate> ReadHandle::deserializeRegion3d(Region3d &region3d) {
        TRY(deserializePaletteTable());
        for (size_t segment3dIndex = 0; segment3dIndex < SEGMENTS_PER_REGION; ++segment3dIndex) {
            if (const auto hasOption = TRY(readByte(EXPECTED_SEGMENT_INDICATOR)); hasOption) {
                region3d.segments[segment3dIndex] = TRY(deserializeSegment3d());
            }
        }
        return {};
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
        TRY(handle.validateZVCRFilePrefix(ZVCR3_FILE_PREFIX));

        const auto version = TRY(handle.deserializeVersion(ZVCR3_VER_LATEST));
        const auto dimensionType = TRY(handle.deserializeDimensionType());

        handle.ctx.initialize(version);

        if (handle.ctx.supportDynamicVersioning)
            handle.ctx.protocolVersion = TRY(handle.read<uint16_t>(EXPECTED_PROTOCOL_VERSION));

        Region3d region3d{handle.ctx.protocolVersion};
        TRY(handle.deserializeRegion3d(region3d));

        return ZVCR3File{version, dimensionType, std::move(region3d)};
    }

    template<size_t snapshotLength>
    void WriteHandle::serializeLayer(const Layer2d<snapshotLength>& layer) {
        writeByte(layer.type);
        serializePackedDeltaData<snapshotLength>(layer.deltas);
    }

    template<size_t snapshotLength>
    ReadResult<Layer2d<snapshotLength>> ReadHandle::deserializeLayer() {
        const auto type = TRY(readByte(EXPECTED_LAYER_TYPE));
        auto layer = Layer2d<snapshotLength>{type};
        TRY(deserializePackedDeltaData<snapshotLength>(layer.deltas));

        return std::move(layer);
    }

    template<size_t snapshotLength>
    void WriteHandle::serializeLayers(const LayerContainer2d<snapshotLength>& layers) {
        write<uint64_t>(layers.layers.size());

        for (const auto& layer: layers.layers | std::views::values)
            serializeLayer<snapshotLength>(layer);
    }

    template<size_t snapshotLength>
    ReadResult<std::monostate> ReadHandle::deserializeLayers(LayerContainer2d<snapshotLength> &layers) {
        const auto layersLength = TRY(read<uint64_t>(EXPECTED_LAYERS_LENGTH));

        for (size_t layerIndex = 0; layerIndex < layersLength; ++layerIndex) {
            auto layer = TRY(deserializeLayer<snapshotLength>());
            layers.layers[layer.type] = layer;
        }
        return {};
    }

    ReadResult<std::monostate> ReadHandle::deserializeBlockLayers(Segment2d &segment2d) {
        return deserializeLayers<SECTION_2D_SIZE_BLOCKS>(segment2d.layers);
    }

    ReadResult<std::monostate> ReadHandle::deserializeBiomeLayers(Segment2d &segment2d) {
        if (ctx.supportBiomes)
            return deserializeLayers<SECTION_2D_SIZE_BIOMES>(segment2d.biomeLayers);

        return {};
    }

    void WriteHandle::serializeSegment2d(const Segment2d& segment) {
        serializeLayers(segment.layers);

        if (ctx.supportBiomes)
            serializeLayers(segment.biomeLayers);

        serializeSegmentInfo(segment.info);
    }

    ReadResult<std::shared_ptr<Segment2d>> ReadHandle::deserializeSegment2d() {
        auto segment = std::make_shared<Segment2d>();
        segment->supportBiomes = ctx.supportBiomes;
        TRY(deserializeBlockLayers(*segment));
        TRY(deserializeBiomeLayers(*segment));
        segment->info = TRY(deserializeSegmentInfo());
        return segment;
    }

    void WriteHandle::serializeOptSegment2d(const Segment2d *segment) {
        if (!segment) {
            writeByte(0);
            return;
        }
        writeByte(1);
        serializeSegment2d(*segment);
    }

    void WriteHandle::serializeRegion2d(const Region2d& region) {
        WriteHandle regionDataHandle{};
        regionDataHandle.ctx = ctx;

        for (size_t i = 0; i < SEGMENTS_PER_REGION; i++)
            regionDataHandle.serializeOptSegment2d(region.segments[i].get());

        if (ctx.supportDynamicVersioning)
            write<uint16_t>(ctx.protocolVersion);

        serializePaletteTable(regionDataHandle.paletteTableStorage);
        writeBytes(regionDataHandle.data);
    }

    ReadResult<std::monostate> ReadHandle::deserializeRegion2d(Region2d &region2d) {
        TRY(deserializePaletteTable());
        for (size_t segmentIndex = 0; segmentIndex < SEGMENTS_PER_REGION; ++segmentIndex) {
            if (const auto hasOption = TRY(readByte(EXPECTED_SEGMENT_INDICATOR)); hasOption) {
                region2d.segments[segmentIndex] = TRY(deserializeSegment2d());
            }
        }
        return {};
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
        TRY(handle.validateZVCRFilePrefix(ZVCR2_FILE_PREFIX));

        const auto version = TRY(handle.deserializeVersion(ZVCR2_VER_LATEST));
        const auto dimensionType = TRY(handle.deserializeDimensionType());

        handle.ctx.initialize(version);

        if (handle.ctx.supportDynamicVersioning)
            handle.ctx.protocolVersion = TRY(handle.read<uint16_t>(EXPECTED_PROTOCOL_VERSION));

        Region2d region2d{handle.ctx.protocolVersion};
        TRY(handle.deserializeRegion2d(region2d));

        return ZVCR2File{version, dimensionType, std::move(region2d)};
    }

}
