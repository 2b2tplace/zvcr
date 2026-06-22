#include <zvcr/io/serialize/deserialize.hpp>

namespace zvcr {

    auto ReadError::what() const -> std::string {
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

    auto ReadError::attach(const ReadHandle &handle) -> void {
        if (handle.data.empty()) return;

        dataLength = handle.data.size();
        sliceStart = offset >= DBG_SLICE_SIZE ? offset - DBG_SLICE_SIZE : 0;
        sliceEnd = std::min(offset + DBG_SLICE_SIZE + 1, dataLength);
        dumpPoint = offset - sliceStart;
        dumpSlice = std::vector(handle.data.begin() + static_cast<int64_t>(sliceStart), handle.data.begin() + static_cast<int64_t>(sliceEnd));
    }

    auto ReadHandle::deserializeDimensionType() -> ReadResult<DimensionType> {
        const auto dimensionTypeId = TRY(readByte(EXPECTED_DIMENSION_TYPE));
        static constexpr auto maxDimensionTypeId = static_cast<uint8_t>(DimensionType::THE_END);

        if (dimensionTypeId > maxDimensionTypeId) {
            const auto err = ReadError{INVALID_DIMENSION_TYPE, offset, "Invalid dimension type: "
                + std::to_string(dimensionTypeId) + " > " + std::to_string(maxDimensionTypeId)};

            return ERR(err);
        }
        const auto dimensionType = static_cast<DimensionType>(dimensionTypeId);
        ctx.initializeSectionCount(dimensionType);

        return dimensionType;
    }

    auto ReadHandle::validateFilePrefix(const std::string_view prefix) -> ReadResult<std::monostate> {
        TRY(skip<uint8_t>(prefix.length(), MISSING_HEADER));

        for (size_t i = 0; i < prefix.length(); ++i) {
            if (const auto expected = static_cast<uint8_t>(prefix[i]); data[i] != expected)
                return ERR(ReadError(INVALID_HEADER_PREFIX, offset, "Invalid header prefix: "
                    + std::to_string(data[i]) + " != " + std::to_string(expected)));
        }
        return {};
    }

    auto ReadHandle::deserializePaletteTable(std::vector<Palette> &paletteTable) -> ReadResult<std::monostate> {
        const auto paletteTableLength = TRY(read<uint32_t>(EXPECTED_PALETTE_TABLE_LENGTH));
        if (paletteTableLength > MAX_PALETTE_TABLE_LENGTH)
            return ERR(ReadError(INVALID_PALETTE_TABLE_LENGTH, offset, "Invalid palette table length: "
                + std::to_string(paletteTableLength) + " > " + std::to_string(MAX_PALETTE_TABLE_LENGTH)));

        paletteTable.reserve(paletteTableLength);
        for (size_t i = 0; i < paletteTableLength; ++i) {
            const auto paletteLength = static_cast<size_t>(TRY(read<uint16_t>(EXPECTED_PALETTE_LENGTH)));
            // direct palette update; needed for backwards compat
            if (paletteLength > MAX_INDIRECT_PALETTE_SIZE) {
                TRY(skip<SegmentAtom>(paletteLength, EXPECTED_PALETTE_DATA));
                paletteTable.push_back(DIRECT_PALETTE);
                continue;
            }
            VectorPalette palette{};
            if (paletteLength > palette.size())
                palette.resize(paletteLength);

            TRY(readArray(palette.data(), paletteLength, EXPECTED_PALETTE_DATA));

            if (ctx.legacyVersion) {
                paletteTable.emplace_back(palette, bitsPerEntryLegacy(paletteLength));
            } else {
                paletteTable.emplace_back(palette, bitsPerEntry(paletteLength));
            }
        }
        return {};
    }

    auto ReadHandle::skipPackedSnapshot() -> ReadResult<std::monostate> {
        TRY(skip<uint64_t>(EXPECTED_TIMESTAMP));

        if (ctx.supportSingleValuePalette) {
            const auto dataType = TRY(read<uint8_t>(EXPECTED_PALETTE_TYPE));
            if (dataType == 0) {
                TRY(skip<uint16_t>(EXPECTED_PALETTE_SINGLE_DATA));
                return {};
            }
        }
        const auto packedLength = TRY(read<uint64_t>(EXPECTED_PACKED_LENGTH));
        if (packedLength > MAX_PACKED_LENGTH) {
            const auto err = ReadError{INVALID_PACKED_LENGTH, offset, "Invalid packed length: "
                + std::to_string(packedLength) + " > " + std::to_string(MAX_PACKED_LENGTH)};
            return ERR(err);
        }
        TRY(skip<uint64_t>(packedLength, EXPECTED_PACKED_DATA));
        TRY(skip<uint32_t>(EXPECTED_PALETTE_INDEX));
        return {};
    }

    auto ReadHandle::deserializeSegmentState() -> ReadResult<SegmentState> {
        const auto stateTypeId = TRY(readByte(EXPECTED_SEGMENT_STATE_TYPE));
        if (stateTypeId > 2) {
            const auto err = ReadError{INVALID_SEGMENT_STATE_ID, offset, "Invalid segment state id: "
                + std::to_string(stateTypeId) + " > 2"};
            return ERR(err);
        }
        const auto stateType = static_cast<SegmentStateType>(stateTypeId);
        const auto timestamp = static_cast<time_t>(TRY(read<uint64_t>(EXPECTED_SEGMENT_STATE_TIMESTAMP)));

        return SegmentState{stateType, timestamp};
    }

    [[nodiscard]]
    static constexpr auto getTotalTileEntities(const uint16_t protocolVersion) -> size_t {
        if (protocolVersion >= 768) return 45;
        if (protocolVersion >= 766) return 44;
        if (protocolVersion >= 765) return 41;

        return -1; // versions pre 1.20.4 are not supported
    }

    auto ReadHandle::deserializeTileEntityCountInfo() -> ReadResult<std::monostate> {
        const auto totalTileEntities = getTotalTileEntities(ctx.protocolVersion);

        TRY(skip<uint16_t>(totalTileEntities, EXPECTED_LEGACY_TILE_ENTITY_COUNTS));
        TRY(read<uint64_t>(EXPECTED_LEGACY_TILE_ENTITY_COUNTS_TIMESTAMP));

        return {};
    }

    auto ReadHandle::deserializeSegmentInfo() -> ReadResult<SegmentInfo> {
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

        if (ctx.legacyVersion) {
            const auto tileEntitiesLength = TRY(read<uint64_t>(EXPECTED_LEGACY_TILE_ENTITIES_LENGTH));
            if (tileEntitiesLength > MAX_LEGACY_TILE_ENTITIES_LENGTH) {
                const auto err = ReadError{INVALID_LEGACY_TILE_ENTITIES_LENGTH, offset, "Invalid tile entities length: "
                    + std::to_string(tileEntitiesLength) + " > " + std::to_string(MAX_LEGACY_TILE_ENTITIES_LENGTH)};
                return ERR(err);
            }
            for (size_t i = 0; i < tileEntitiesLength; ++i)
                TRY(deserializeTileEntityCountInfo());
        }
        return SegmentInfo{states};
    }

    auto ReadHandle::deserializeTileEntities() -> ReadResult<DeltaTileEntityData> {
        const auto tileEntityDeltasLength = TRY(read<uint64_t>(EXPECTED_TILE_ENTITY_LIST_DELTAS_LENGTH));
        if (tileEntityDeltasLength > MAX_DELTA_LENGTH) {
            const auto err = ReadError{INVALID_TILE_ENTITY_LIST_DELTAS_LENGTH, offset, "Invalid tile entity list deltas length: "
                + std::to_string(tileEntityDeltasLength) + " > " + std::to_string(MAX_DELTA_LENGTH)};
            return ERR(err);
        }
        DeltaTileEntityData tileEntities;
        tileEntities.reverseDeltas.reserve(tileEntityDeltasLength);

        for (size_t i = 0; i < tileEntityDeltasLength; ++i) {
            const auto timestamp = TRY(read<uint64_t>(EXPECTED_TILE_ENTITY_LIST_TIMESTAMP));
            const auto tileEntityListLength = TRY(read<uint64_t>(EXPECTED_TILE_ENTITY_LIST_LENGTH));

            if (tileEntityListLength > MAX_TILE_ENTITY_LIST_LENGTH) {
                const auto err = ReadError{INVALID_TILE_ENTITY_LIST_LENGTH, offset, "Invalid tile entity list length: "
                    + std::to_string(tileEntityListLength) + " > " + std::to_string(MAX_TILE_ENTITY_LIST_LENGTH)};
                return ERR(err);
            }
            auto &[_, deltas] = tileEntities.reverseDeltas.emplace_back(static_cast<time_t>(timestamp));
            for (size_t j = 0; j < tileEntityListLength; ++j) {
                const auto pos = TileEntityPosition::unpack(TRY(read<uint32_t>(EXPECTED_TILE_ENTITY_PACKED_POSITION)));
                if (const auto put = TRY(read<uint8_t>(EXPECTED_TILE_ENTITY_DELTA_OPERATION)); !put) {
                    deltas[pos] = std::monostate{};
                    continue;
                }
                const auto type = TRY(read<uint32_t>(EXPECTED_TILE_ENTITY_TYPE));
                const auto nbtLength = TRY(read<uint64_t>(EXPECTED_TILE_ENTITY_NBT_LENGTH));
                if (nbtLength > MAX_TILE_ENTITY_NBT_LENGTH) {
                    const auto err = ReadError{INVALID_TILE_ENTITY_NBT_LENGTH, offset, "Invalid tile entity nbt length: "
                        + std::to_string(nbtLength) + " > " + std::to_string(MAX_TILE_ENTITY_NBT_LENGTH)};
                    return ERR(err);
                }
                TileEntity tileEntity{.type = type, .pos = pos};
                tileEntity.nbt.resize(nbtLength);
                TRY(readArray<uint8_t>(tileEntity.nbt, EXPECTED_TILE_ENTITY_NBT));
                deltas[pos] = tileEntity;
            }
        }
        return tileEntities;
    }

    auto ReadHandle::deserializeSegment() -> ReadResult<std::shared_ptr<Segment>> {
        auto segment = std::make_shared<Segment>(ctx.sectionCount, ctx.supportBiomes);

        for (size_t sectionIndex = 0; sectionIndex < ctx.sectionCount; ++sectionIndex)
            TRY(deserializePackedDeltaData<SECTION_SIZE_BLOCKS>(segment->blockSections.sections[sectionIndex], blockPaletteTable));

        if (ctx.supportBiomes) {
            for (size_t sectionIndex = 0; sectionIndex < ctx.sectionCount; ++sectionIndex)
                TRY(deserializePackedDeltaData<SECTION_SIZE_BIOMES>(segment->biomeSections.sections[sectionIndex], ctx.legacyVersion ? blockPaletteTable : biomePaletteTable));
        }
        segment->info = TRY(deserializeSegmentInfo());
        if (ctx.supportTileEntities)
            segment->tileEntities = TRY(deserializeTileEntities());

        return segment;
    }

    auto ReadHandle::deserializeRegion(Region &region) -> ReadResult<std::monostate> {
        TRY(deserializePaletteTable(blockPaletteTable));
        if (!ctx.legacyVersion)
            TRY(deserializePaletteTable(biomePaletteTable));

        for (size_t segmentIndex = 0; segmentIndex < SEGMENTS_PER_REGION; ++segmentIndex) {
            if (const auto hasOption = TRY(readByte(EXPECTED_SEGMENT_INDICATOR)); hasOption) {
                region.segments[segmentIndex] = TRY(deserializeSegment());
            }
        }
        return {};
    }

    auto deserializeFileLegacy(ReadHandle &handle) -> ReadResult<File> {
        TRY(handle.validateFilePrefix(legacyFilePrefix));

        const auto version = TRY(handle.deserializeVersion(latestVersion));
        const auto dimensionType = TRY(handle.deserializeDimensionType());

        handle.ctx.initialize(version);

        if (handle.ctx.supportDynamicVersioning)
            handle.ctx.protocolVersion = TRY(handle.read<uint16_t>(EXPECTED_PROTOCOL_VERSION));

        File file{
            .version = version,
            .protocolVersion = handle.ctx.protocolVersion,
            .dimensionType = dimensionType
        };
        const auto result = handle.deserializeRegion(file.region);
        TRY(result);

        return file;
    }

    auto deserializeFile(ReadHandle &handle) -> ReadResult<File> {
        TRY(handle.validateFilePrefix(filePrefix));

        const auto version = TRY(handle.deserializeVersion(latestVersion));
        const auto dimensionType = TRY(handle.deserializeDimensionType());

        handle.ctx.initialize(version);

        if (handle.ctx.supportDynamicVersioning)
            handle.ctx.protocolVersion = TRY(handle.read<uint16_t>(EXPECTED_PROTOCOL_VERSION));

        File file{
            .version = version,
            .protocolVersion = handle.ctx.protocolVersion,
            .dimensionType = dimensionType
        };
        const auto compressed = std::vector<uint8_t>{handle.data.begin() + static_cast<std::ptrdiff_t>(handle.getOffset()), handle.data.end()};
        std::vector<uint8_t> uncompressed;
        const auto decompressResult = decompressZstd(compressed, uncompressed);
        if (!decompressResult) {
            const auto err = ReadError{ZSTD_ERROR, 0, fmt::format("Failed to decompress region container: {}", decompressResult.error())};
            return ERR(err);
        }
        ReadHandle regionReadHandle{std::move(uncompressed), handle.maxDeltas};
        regionReadHandle.ctx = handle.ctx;
        const auto result = regionReadHandle.deserializeRegion(file.region);
        TRY(result);

        return file;
    }

    auto readFileLegacy(const fs::path &filepath, const size_t maxDeltas) -> ReadResult<File> {
        std::ifstream fileStream(filepath, std::ios::in | std::ios::binary);
        if (!fileStream)
            return ERR(ReadError(FILE_NOT_FOUND, 0, fmt::format("Failed to open file: ", filepath.string())));

        ZSTD_DStream *dstream = ZSTD_createDStream();
        if (!dstream)
            return ERR(ReadError(GENERIC_READ_ERROR, 0, "Failed to create ZSTD_DStream"));

        auto ret = ZSTD_initDStream(dstream);
        if (ZSTD_isError(ret)) {
            ZSTD_freeDStream(dstream);
            return ERR(ReadError(GENERIC_READ_ERROR, 0, fmt::format("ZSTD_initDStream error: ", ZSTD_getErrorName(ret))));
        }
        const auto inChunkSize = ZSTD_DStreamInSize();
        const auto outChunkSize = ZSTD_DStreamOutSize();

        std::vector<char> inBuffer(inChunkSize);
        std::vector<char> outBuffer(outChunkSize);
        std::vector<uint8_t> decompressed;
        decompressed.reserve(preallocate);

        while (true) {
            fileStream.read(inBuffer.data(), static_cast<int64_t>(inChunkSize));
            std::streamsize bytesRead = fileStream.gcount();
            if (bytesRead == 0) break;

            ZSTD_inBuffer input = { inBuffer.data(), static_cast<size_t>(bytesRead), 0 };

            while (input.pos < input.size) {
                ZSTD_outBuffer output = { outBuffer.data(), outBuffer.size(), 0 };
                ret = ZSTD_decompressStream(dstream, &output, &input);
                if (ZSTD_isError(ret)) {
                    ZSTD_freeDStream(dstream);
                    return ERR(ReadError(GENERIC_READ_ERROR, 0, fmt::format("ZSTD_decompressStream error: ", ZSTD_getErrorName(ret))));
                }
                decompressed.insert(decompressed.end(), outBuffer.data(), outBuffer.data() + static_cast<int64_t>(output.pos));
            }
        }
        ZSTD_freeDStream(dstream);
        ReadHandle handle{std::move(decompressed), maxDeltas};

        try {
            auto result = deserializeFileLegacy(handle);

            if (!result.has_value()) {
                auto err = result.error();
                err.attach(handle);
                return ERR(err);
            }
            return result;
        } catch (const std::exception &e) {
            auto err = ReadError{GENERIC_READ_ERROR, handle.getOffset(), fmt::format("Generic read error: ", e.what())};
            err.attach(handle);
            return ERR(err);
        }
    }

    auto readFile(const fs::path &filepath, const size_t maxDeltas) -> ReadResult<File> {
        std::ifstream fileStream(filepath, std::ios::binary | std::ios::ate);
        if (!fileStream)
            return ERR(ReadError(FILE_NOT_FOUND, 0, fmt::format("Failed to open file '{}'", filepath.string())));

        const auto size = fileStream.tellg();
        fileStream.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(size);

        if (!fileStream.read(reinterpret_cast<char*>(buffer.data()), size))
            return ERR(ReadError(GENERIC_READ_ERROR, 0, fmt::format("Failed to read file '{}'", filepath.string())));

        ReadHandle handle{std::move(buffer), maxDeltas};

        try {
            auto result = deserializeFile(handle);

            if (!result.has_value()) {
                auto err = result.error();
                err.attach(handle);
                return ERR(err);
            }
            return result;
        } catch (const std::exception &e) {
            auto err = ReadError{GENERIC_READ_ERROR, handle.getOffset(), fmt::format("Generic read error: {}", e.what())};
            err.attach(handle);
            return ERR(err);
        }
    }

    auto readFileAtLegacy(const fs::path &parentDirectory, const RegionLocation &location, const size_t maxDeltas) -> ReadResult<File> {
        return readFileLegacy(location.filePathLegacy(parentDirectory), maxDeltas);
    }

    auto readFileAt(const fs::path &parentDirectory, const RegionLocation &location, const size_t maxDeltas) -> ReadResult<File> {
        return readFile(location.filePath(parentDirectory), maxDeltas);
    }
}
