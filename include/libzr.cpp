#include "libzr.hpp"
#include <zstd.h>

#include "try.hpp"

ZResult<ZrDimensionType> deserializeDimensionType(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset >= data.size())
        return std::unexpected(EXPECTED_DIMENSION_TYPE);

    const auto dimensionTypeId = data[offset++];

    if (dimensionTypeId > static_cast<uint8_t>(ZrDimensionType::THE_END))
        return std::unexpected(INVALID_DIMENSION_TYPE);

    return static_cast<ZrDimensionType>(dimensionTypeId);
}

template<typename Version>
ZResult<Version> deserializeVersion(const std::vector<uint8_t>& data, size_t& offset, const Version latest) {
    if (offset >= data.size())
        return std::unexpected(EXPECTED_VERSION);

    const auto versionNumber = data[offset++];

    if (versionNumber > static_cast<uint8_t>(latest))
        return std::unexpected(INVALID_VERSION);

    return static_cast<Version>(versionNumber);
}

std::optional<ZrError> validateZFilePrefix(const std::vector<uint8_t>& data, size_t& offset, const std::string& prefix) {
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
size_t writeZFile(const R& file, const std::string& filename, const ZFileSerialize<R>& serialize) {
    std::vector<uint8_t> bytesUncompressed;
    serialize(file, bytesUncompressed);
    const auto bytesCompressed = compressData(bytesUncompressed);

    std::ofstream fileStream(filename, std::ios::out | std::ios::binary);
    fileStream.write(reinterpret_cast<const char*>(bytesCompressed.data()), bytesCompressed.size());
    fileStream.close();

    return bytesCompressed.size();
}

template<typename R>
ZResult<R> readZFile(const std::string& filename, const ssize_t maxDeltas, const ZFileDeserialize<R>& deserialize) {
    if (!std::filesystem::exists(filename))
        return std::unexpected(FILE_NOT_FOUND);

    try {
        std::ifstream fileStream(filename, std::ios::in | std::ios::binary);

        fileStream.seekg(0, std::ios::end);
        const size_t fileSize = fileStream.tellg();
        fileStream.seekg(0, std::ios::beg);

        const auto bytesCompressed = new char[fileSize];
        fileStream.read(bytesCompressed, fileSize);
        fileStream.close();

        const auto bytesCompressedVector = std::vector<uint8_t>(bytesCompressed, bytesCompressed + fileSize);
        const auto bytesUncompressed = decompressData(bytesCompressedVector);
        size_t offset{};

        delete bytesCompressed;
        return deserialize(bytesUncompressed, offset, maxDeltas);
    } catch (const std::length_error&) {
        return std::unexpected(GENERIC_READ_ERROR);
    }
}

void serializeBlockStatesSnapshot(const ZrBlockStatesSnapshot& snapshot, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
    data.resize(data.size() + sizeof(time_t));
    std::memcpy(data.data() + data.size() - sizeof(time_t), &snapshot.timestamp, sizeof(time_t));

    const LongArray packedData = snapshot.data.packedData;
    const uint64_t packedLength = packedData.size();
    const uint64_t packedSize = packedLength * sizeof(uint64_t);

    data.resize(data.size() + sizeof(uint64_t) + packedLength * sizeof(uint64_t));
    std::memcpy(data.data() + data.size() - sizeof(uint64_t) - packedSize, &packedLength, sizeof(uint64_t));
    std::memcpy(data.data() + data.size() - packedSize, packedData.data(), packedSize);

    const Palette palette = snapshot.data.palette;
    size_t paletteIndex = paletteTable.size();
    for (size_t i = 0; i < paletteTable.size(); ++i) {
        if (const auto existingPalette = paletteTable[i]; palette == existingPalette) {
            paletteIndex = i;
            break;
        }
    }
    if (paletteIndex == paletteTable.size())
        paletteTable.push_back(palette);

    data.resize(data.size() + sizeof(uint32_t));
    std::memcpy(data.data() + data.size() - sizeof(uint32_t), &paletteIndex, sizeof(uint32_t));
}

ZResult<ZrBlockStatesSnapshot> deserializeBlockStatesSnapshot(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const size_t snapshotLength) {
    if (offset + sizeof(time_t) > data.size())
        return std::unexpected(EXPECTED_TIMESTAMP);

    time_t timestamp;
    std::memcpy(&timestamp, data.data() + offset, sizeof(time_t));
    offset += sizeof(time_t);

    if (offset + sizeof(uint64_t) > data.size())
        return std::unexpected(EXPECTED_PACKED_LENGTH);

    uint64_t packedLength;
    std::memcpy(&packedLength, data.data() + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    if (offset + packedLength * sizeof(uint64_t) > data.size())
        return std::unexpected(EXPECTED_PACKED_DATA);

    LongArray packedData;
    packedData.resize(packedLength);
    std::memcpy(packedData.data(), data.data() + offset, packedLength * sizeof(uint64_t));
    offset += packedLength * sizeof(uint64_t);

    if (offset + sizeof(uint32_t) > data.size())
        return std::unexpected(EXPECTED_PALETTE_INDEX);

    uint32_t paletteIndex;
    std::memcpy(&paletteIndex, data.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    const auto palette = paletteTable[paletteIndex];
    return ZrBlockStatesSnapshot {
        ZrBlockStates(palette, packedData, snapshotLength),
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

ZResult<std::vector<Palette>> deserializePaletteTable(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + sizeof(uint32_t) > data.size())
        return std::unexpected(EXPECTED_PALETTE_TABLE_LENGTH);

    uint32_t paletteTableLength;
    std::memcpy(&paletteTableLength, data.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    std::vector<Palette> paletteTable;
    for (uint32_t i = 0; i < paletteTableLength; ++i) {
        if (offset + sizeof(uint16_t) > data.size())
            return std::unexpected(EXPECTED_PALETTE_LENGTH);

        uint16_t paletteLength;
        std::memcpy(&paletteLength, data.data() + offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        const auto paletteLengthSize = static_cast<size_t>(paletteLength);
        if (offset + paletteLengthSize * sizeof(uint16_t) > data.size())
            return std::unexpected(EXPECTED_PALETTE_DATA);

        Palette palette;
        palette.resize(paletteLengthSize);
        std::memcpy(palette.data(), data.data() + offset, paletteLengthSize * sizeof(uint16_t));
        offset += paletteLengthSize * sizeof(uint16_t);
        paletteTable.emplace_back(std::move(palette));
    }
    return paletteTable;
}

std::optional<ZrError> skipBlockStatesSnapshot(const std::vector<uint8_t>& data, size_t& offset) {
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

void serializeBlockStates(const ZrDeltaBlockStates& chunkSection, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
    const uint64_t deltaLength = chunkSection.reverseDeltas.size();
    data.resize(data.size() + sizeof(uint64_t));
    std::memcpy(data.data() + data.size() - sizeof(uint64_t), &deltaLength, sizeof(uint64_t));

    for (const ZrBlockStatesSnapshot& foo : chunkSection.reverseDeltas)
        serializeBlockStatesSnapshot(foo, data, paletteTable);
}

ZResult<ZrDeltaBlockStates> deserializeBlockStates(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const ssize_t maxDeltas, const size_t snapshotLength) {
    if (offset + sizeof(uint64_t) > data.size())
        return std::unexpected(EXPECTED_DELTA_LENGTH);

    size_t deltaLength;
    std::memcpy(&deltaLength, data.data() + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    std::vector<ZrBlockStatesSnapshot> reverseDeltas;
    reverseDeltas.reserve(deltaLength);

    for (size_t deltaIndex = 0; deltaIndex < deltaLength; ++deltaIndex) {
        if (maxDeltas != -1 && deltaIndex > maxDeltas) {
            Propagate(skipBlockStatesSnapshot(data, offset));
            continue;
        }
        const auto snapshot = Try(deserializeBlockStatesSnapshot(data, offset, paletteTable, snapshotLength));
        reverseDeltas.push_back(snapshot);
    }
    return ZrDeltaBlockStates(reverseDeltas, snapshotLength);
}

void serializeChunkState(const ZrChunkState& chunkState, std::vector<uint8_t>& data) {
    data.push_back(static_cast<uint8_t>(chunkState.type));
    data.resize(data.size() + sizeof(time_t));
    std::memcpy(data.data() + data.size() - sizeof(time_t), &chunkState.timestamp, sizeof(time_t));
}

ZResult<ZrChunkState> deserializeChunkState(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset >= data.size())
        return std::unexpected(EXPECTED_CHUNK_STATE_TYPE);

    const auto type = static_cast<ZrChunkStateType>(data[offset++]);

    if (offset + sizeof(time_t) > data.size())
        return std::unexpected(EXPECTED_CHUNK_STATE_TIMESTAMP);

    time_t timestamp;
    std::memcpy(&timestamp, data.data() + offset, sizeof(time_t));
    offset += sizeof(time_t);

    return ZrChunkState {type, timestamp};
}

void serializeTileEntityCounts(const ZrTileEntityCounts& tileEntityCounts, std::vector<uint8_t>& data) {
    data.resize(data.size() + sizeof(uint16_t) * TILE_ENTITIES);
    std::memcpy(data.data() + data.size() - sizeof(uint16_t) * TILE_ENTITIES, tileEntityCounts.counts.data(), sizeof(uint16_t) * TILE_ENTITIES);
    data.resize(data.size() + sizeof(time_t));
    std::memcpy(data.data() + data.size() - sizeof(time_t), &tileEntityCounts.timestamp, sizeof(time_t));
}

ZResult<ZrTileEntityCounts> deserializeTileEntityCounts(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + sizeof(uint16_t) * TILE_ENTITIES > data.size())
        return std::unexpected(EXPECTED_TILE_ENTITY_COUNTS);
    
    std::vector<uint16_t> counts(TILE_ENTITIES);
    std::memcpy(counts.data(), data.data() + offset, sizeof(uint16_t) * TILE_ENTITIES);
    offset += sizeof(uint16_t) * TILE_ENTITIES;
    
    if (offset + sizeof(time_t) > data.size())
        return std::unexpected(EXPECTED_TILE_ENTITY_COUNTS_TIMESTAMP);
    
    time_t timestamp;
    std::memcpy(&timestamp, data.data() + offset, sizeof(time_t));
    offset += sizeof(time_t);

    return ZrTileEntityCounts {counts, timestamp};
}

void serializeSector(const ZrSector& sector, std::vector<uint8_t>& data) {
    const uint64_t statesLength = sector.chunkStates.size();
    data.resize(data.size() + sizeof(uint64_t));
    std::memcpy(data.data() + data.size() - sizeof(uint64_t), &statesLength, sizeof(uint64_t));

    for (const ZrChunkState& state : sector.chunkStates)
        serializeChunkState(state, data);

    const size_t tileEntitiesLength = sector.tileEntities.size();
    data.resize(data.size() + sizeof(uint64_t));
    std::memcpy(data.data() + data.size() - sizeof(uint64_t), &tileEntitiesLength, sizeof(uint64_t));

    for (const ZrTileEntityCounts& tiles : sector.tileEntities)
        serializeTileEntityCounts(tiles, data);
}

ZResult<ZrSector> deserializeSector(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + sizeof(uint64_t) > data.size())
        return std::unexpected(EXPECTED_CHUNK_STATES_LENGTH);

    uint64_t statesLength;
    std::memcpy(&statesLength, data.data() + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    ChunkStates states;
    states.reserve(statesLength);

    for (size_t stateIndex = 0; stateIndex < statesLength; ++stateIndex) 
        states.push_back(Try(deserializeChunkState(data, offset)));

    if (offset + sizeof(uint64_t) > data.size())
        return std::unexpected(EXPECTED_TILE_ENTITIES_LENGTH);

    uint64_t tileEntitiesLength;
    std::memcpy(&tileEntitiesLength, data.data() + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    TileEntities tileEntities;
    tileEntities.reserve(tileEntitiesLength);

    for (size_t tileEntityIndex = 0; tileEntityIndex < tileEntitiesLength; ++tileEntityIndex)
        tileEntities.push_back(Try(deserializeTileEntityCounts(data, offset)));

    return ZrSector(states, tileEntities);
}

void serializeChunk(const ZvrChunk& chunk, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
    for (const ZrDeltaBlockStates& section : chunk.sections)
        serializeBlockStates(section, data, paletteTable);

    serializeSector(chunk, data);
}

ZResult<ZvrChunk> deserializeChunk(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const ssize_t maxDeltas, const uint32_t sectionAmount) {
    Sections sections;
    sections.reserve(sectionAmount);

    for (size_t sectionIndex = 0; sectionIndex < sectionAmount; ++sectionIndex)
        sections.push_back(Try(deserializeBlockStates(data, offset, paletteTable, maxDeltas, SECTION_SIZE)));

    const auto sector = Try(deserializeSector(data, offset));
    return ZvrChunk(sections, sector.chunkStates, sector.tileEntities);
}

void serializeOptionalChunk(const std::optional<ZvrChunk>& chunk, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
    if (!chunk.has_value()) {
        data.push_back(0);
        return;
    }
    data.push_back(1);
    serializeChunk(chunk.value(), data, paletteTable);
}

ZResult<std::optional<ZvrChunk>> deserializeOptionalChunk(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const ssize_t maxDeltas, const uint32_t sectionAmount) {
    if (offset >= data.size())
        return std::unexpected(EXPECTED_CHUNK_INDICATOR);

    if (data[offset++] == 0)
        return std::nullopt;

    return Try(deserializeChunk(data, offset, paletteTable, maxDeltas, sectionAmount));
}

void serializeZVRegion(const ZvrRegion& region, std::vector<uint8_t>& data) {
    std::vector<Palette> paletteTable;
    std::vector<uint8_t> regionData;

    for (const auto& chunk : region.segments)
        serializeOptionalChunk(chunk, regionData, paletteTable);

    serializePaletteTable(paletteTable, data);
    data.insert(data.end(), regionData.begin(), regionData.end());
}

ZResult<ZvrRegion> deserializeZVRegion(const std::vector<uint8_t>& data, size_t& offset, const ssize_t maxDeltas, const uint32_t sectionAmount) {
    const auto paletteTable = Try(deserializePaletteTable(data, offset));
    Chunks chunks(SEGMENTS_PER_REGION);
    for (size_t chunkIndex = 0; chunkIndex < SEGMENTS_PER_REGION; ++chunkIndex)
        chunks[chunkIndex] = Try(deserializeOptionalChunk(data, offset, paletteTable, maxDeltas, sectionAmount));

    return ZvrRegion(chunks);
}

void serializeZVRFile(const ZvrFile& file, std::vector<uint8_t>& data) {
    const std::string prefix = ZVR_PREFIX;
    data.insert(data.end(), prefix.begin(), prefix.end());
    data.push_back(static_cast<uint8_t>(file.version));
    data.push_back(static_cast<uint8_t>(file.dimensionType));
    serializeZVRegion(file.region, data);
}

ZResult<ZvrFile> deserializeZVRFile(const std::vector<uint8_t>& data, size_t& offset, const ssize_t maxDeltas) {
    validateZFilePrefix(data, offset, ZVR_PREFIX);
    const auto version = Try(deserializeVersion(data, offset, ZVR_LATEST));
    const auto dimensionType = Try(deserializeDimensionType(data, offset));

    const auto sectionAmount = DimensionTypePropertyRegistry.at(dimensionType).height / CHUNK_SIDELENGTH;
    const auto region = Try(deserializeZVRegion(data, offset, maxDeltas, sectionAmount));

    return ZvrFile {version, dimensionType, region};
}

size_t writeZVRFile(const ZvrFile& file, const std::string& filename) {
    return writeZFile(file, filename, ZFileSerialize(serializeZVRFile));
}

ZResult<ZvrFile> readZVRFile(const std::string& filename, const ssize_t maxDeltas) {
    return readZFile(filename, maxDeltas, ZFileDeserialize(deserializeZVRFile));
}

void serializeLayer(const ZprLayer& layer, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
    data.push_back(layer.type);
    serializeBlockStates(layer.deltas, data, paletteTable);
}

ZResult<ZprLayer> deserializeLayer(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const ssize_t maxDeltas) {
    if (offset >= data.size())
        return std::unexpected(EXPECTED_LAYER_TYPE);

    const auto type = data[offset++];
    const auto deltas = Try(deserializeBlockStates(data, offset, paletteTable, maxDeltas, TILE_SIZE));

    return ZprLayer {deltas, type};
}

void serializeLayers(const ZprLayers& layers, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
    const uint64_t layersLength = layers.size();
    data.resize(data.size() + sizeof(uint64_t));
    std::memcpy(data.data() + data.size() - sizeof(uint64_t), &layersLength, sizeof(uint64_t));

    for (const auto &layer: layers | std::views::values)
        serializeLayer(layer, data, paletteTable);
}

ZResult<ZprLayers> deserializeLayers(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const ssize_t maxDeltas) {
    if (offset + sizeof(uint64_t) > data.size())
        return std::unexpected(EXPECTED_LAYERS_LENGTH);

    uint64_t layersLength;
    std::memcpy(&layersLength, data.data() + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);

    ZprLayers layers;
    for (size_t layerIndex = 0; layerIndex < layersLength; ++layerIndex) {
        const auto layer = Try(deserializeLayer(data, offset, paletteTable, maxDeltas));
        layers[layer.type] = layer;
    }
    return layers;
}

void serializeSegment(const ZprSegment& segment, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
    serializeLayers(segment.layers, data, paletteTable);
    serializeSector(segment, data);
}

ZResult<ZprSegment> deserializeSegment(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const ssize_t maxDeltas) {
    const auto layers = Try(deserializeLayers(data, offset, paletteTable, maxDeltas));
    const auto sector = Try(deserializeSector(data, offset));

    return ZprSegment {layers, sector.chunkStates, sector.tileEntities};
}

void serializeOptionalSegment(const std::optional<ZprSegment>& segment, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable) {
    if (!segment.has_value()) {
        data.push_back(0);
        return;
    }
    data.push_back(1);
    serializeSegment(segment.value(), data, paletteTable);
}

ZResult<std::optional<ZprSegment>> deserializeOptionalSegment(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, const ssize_t maxDeltas) {
    if (offset >= data.size())
        return std::unexpected(EXPECTED_CHUNK_INDICATOR);

    if (data[offset++] == 0)
        return std::nullopt;

    return Try(deserializeSegment(data, offset, paletteTable, maxDeltas));
}

void serializeZPRegion(const ZprRegion& region, std::vector<uint8_t>& data) {
    std::vector<Palette> paletteTable;
    std::vector<uint8_t> regionData;

    for (const auto& segment : region.segments)
        serializeOptionalSegment(segment, regionData, paletteTable);

    serializePaletteTable(paletteTable, data);
    data.insert(data.end(), regionData.begin(), regionData.end());
}

ZResult<ZprRegion> deserializeZPRegion(const std::vector<uint8_t>& data, size_t& offset, const ssize_t maxDeltas) {
    const auto paletteTable = Try(deserializePaletteTable(data, offset));
    Segments chunks(SEGMENTS_PER_REGION);
    for (size_t segmentIndex = 0; segmentIndex < SEGMENTS_PER_REGION; ++segmentIndex)
        chunks[segmentIndex] = Try(deserializeOptionalSegment(data, offset, paletteTable, maxDeltas));

    return ZprRegion(chunks);
}

void serializeZPRFile(const ZprFile& file, std::vector<uint8_t>& data) {
    const std::string prefix = ZPR_PREFIX;
    data.insert(data.end(), prefix.begin(), prefix.end());
    data.push_back(static_cast<uint8_t>(file.version));
    data.push_back(static_cast<uint8_t>(file.dimensionType));
    serializeZPRegion(file.region, data);
}

ZResult<ZprFile> deserializeZPRFile(const std::vector<uint8_t>& data, size_t& offset, const ssize_t maxDeltas) {
    validateZFilePrefix(data, offset, ZPR_PREFIX);
    const auto version = Try(deserializeVersion(data, offset, ZPR_LATEST));
    const auto dimensionType = Try(deserializeDimensionType(data, offset));
    const auto region = Try(deserializeZPRegion(data, offset, maxDeltas));

    return ZprFile {version, dimensionType, region};
}

size_t writeZPRFile(const ZprFile& file, const std::string& filename) {
    return writeZFile(file, filename, ZFileSerialize(serializeZPRFile));
}

ZResult<ZprFile> readZPRFile(const std::string& filename, const ssize_t maxDeltas) {
    return readZFile(filename, maxDeltas, ZFileDeserialize(deserializeZPRFile));
}

std::vector<uint8_t> compressData(const std::vector<uint8_t>& inputData) {
    ZSTD_CCtx* cctx = ZSTD_createCCtx();
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, ZSTD_COMPRESSION_THREADS);

    const size_t compressedSize = ZSTD_compressBound(inputData.size());
    std::vector<uint8_t> compressedData(compressedSize);

    const size_t actualCompressedSize = ZSTD_compressCCtx(cctx, compressedData.data(), compressedSize,
        inputData.data(), inputData.size(), ZSTD_COMPRESSION_LEVEL);

    if (ZSTD_isError(actualCompressedSize)) {
        ZSTD_freeCCtx(cctx);
        throw std::runtime_error("ZSTD compression failed: " + std::string(ZSTD_getErrorName(actualCompressedSize)));
    }
    ZSTD_freeCCtx(cctx);
    compressedData.resize(actualCompressedSize);
    return compressedData;
}

std::vector<uint8_t> decompressData(const std::vector<uint8_t>& compressedData) {
    std::vector<uint8_t> decompressedData;
    ZSTD_DCtx* dctx = ZSTD_createDCtx();
    const auto decompressedSize = ZSTD_getFrameContentSize(compressedData.data(), compressedData.size());

    decompressedData.resize(decompressedSize);
    const size_t actualDecompressedSize = ZSTD_decompressDCtx(dctx, decompressedData.data(), decompressedSize,
        compressedData.data(), compressedData.size());

    if (ZSTD_isError(actualDecompressedSize)) {
        ZSTD_freeDCtx(dctx);
        throw std::runtime_error("ZSTD decompression failed: " + std::string(ZSTD_getErrorName(actualDecompressedSize)));
    }
    ZSTD_freeDCtx(dctx);
    return decompressedData;
}