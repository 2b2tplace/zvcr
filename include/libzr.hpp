#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

#include <libzr/modules/common/zr_paletted_storage.hpp>
#include <libzr/modules/zvr/zvr.hpp>
#include <libzr/modules/zpr/zpr.hpp>

enum ZrError {
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
    EXPECTED_CHUNK_INDICATOR,
    EXPECTED_VERSION,
    EXPECTED_DIMENSION_TYPE,
    EXPECTED_CHUNK_STATES_LENGTH,
    EXPECTED_CHUNK_STATE_TYPE,
    EXPECTED_CHUNK_STATE_TIMESTAMP,
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

template<typename R>
using ZResult = std::expected<R, ZrError>;

template<typename R>
using ZFileSerialize = std::function<void(const R&, std::vector<uint8_t>&)>;

template<typename R>
using ZFileDeserialize = std::function<ZResult<R>(const std::vector<uint8_t>&, size_t&, size_t)>;

ZResult<ZrDimensionType> deserializeDimensionType(const std::vector<uint8_t>& data, size_t& offset);

template<typename Version>
ZResult<Version> deserializeVersion(const std::vector<uint8_t>& data, size_t& offset, Version latest);

std::optional<ZrError> validateZFilePrefix(const std::vector<uint8_t>& data, size_t& offset, const std::string& prefix);

template<typename R>
size_t writeZFile(const R& file, const std::string& filename, const ZFileSerialize<R>& serialize);

template<typename R>
ZResult<R> readZFile(const std::string& filename, size_t maxDeltas, const ZFileDeserialize<R>& deserialize);

void serializeBlockStatesSnapshot(const ZrBlockStatesSnapshot& snapshot, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

ZResult<ZrBlockStatesSnapshot> deserializeBlockStatesSnapshot(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t snapshotLength);

void serializePaletteTable(const std::vector<Palette>& paletteTable, std::vector<uint8_t>& data);

ZResult<std::vector<Palette>> deserializePaletteTable(const std::vector<uint8_t>& data, size_t& offset);

std::optional<ZrError> skipBlockStatesSnapshot(const std::vector<uint8_t>& data, size_t& offset);

void serializeBlockStates(const ZrDeltaBlockStates& chunkSection, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

ZResult<ZrDeltaBlockStates> deserializeBlockStates(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t maxDeltas, size_t snapshotLength);

void serializeChunkState(const ZrChunkState& chunkState, std::vector<uint8_t>& data);

ZResult<ZrChunkState> deserializeChunkState(const std::vector<uint8_t>& data, size_t& offset);

void serializeTileEntityCounts(const ZrTileEntityCounts& tileEntityCounts, std::vector<uint8_t>& data);

ZResult<ZrTileEntityCounts> deserializeTileEntityCounts(const std::vector<uint8_t>& data, size_t& offset);

void serializeSector(const ZrSector& sector, std::vector<uint8_t>& data);

ZResult<ZrSector> deserializeSector(const std::vector<uint8_t>& data, size_t& offset);

void serializeChunk(const ZvrChunk& chunk, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

ZResult<ZvrChunk> deserializeChunk(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t maxDeltas, uint32_t sectionAmount);

void serializeOptionalChunk(const std::optional<ZvrChunk>& chunk, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

ZResult<std::optional<ZvrChunk>> deserializeOptionalChunk(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t maxDeltas, uint32_t sectionAmount);

void serializeZVRegion(const ZvrRegion& region, std::vector<uint8_t>& data);

ZResult<ZvrRegion> deserializeZVRegion(const std::vector<uint8_t>& data, size_t& offset, size_t maxDeltas, uint32_t sectionAmount);

void serializeZVRFile(const ZvrFile& file, std::vector<uint8_t>& data);

ZResult<ZvrFile> deserializeZVRFile(const std::vector<uint8_t>& data, size_t& offset, size_t maxDeltas);

size_t writeZVRFile(const ZvrFile& file, const std::string& filename);

ZResult<ZvrFile> readZVRFile(const std::string& filename, size_t maxDeltas);

void serializeLayer(const ZprLayer& layer, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

ZResult<ZprLayer> deserializeLayer(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t maxDeltas);

void serializeLayers(const ZprLayers& layers, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

ZResult<ZprLayers> deserializeLayers(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t maxDeltas);

void serializeSegment(const ZprSegment& segment, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

ZResult<ZprSegment> deserializeSegment(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t maxDeltas);

void serializeOptionalSegment(const std::optional<ZprSegment>& segment, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

ZResult<std::optional<ZprSegment>> deserializeOptionalSegment(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t maxDeltas);

void serializeZPRegion(const ZprRegion& region, std::vector<uint8_t>& data);

ZResult<ZprRegion> deserializeZPRegion(const std::vector<uint8_t>& data, size_t& offset, size_t maxDeltas);

void serializeZPRFile(const ZprFile& file, std::vector<uint8_t>& data);

ZResult<ZprFile> deserializeZPRFile(const std::vector<uint8_t>& data, size_t& offset, size_t maxDeltas);

size_t writeZPRFile(const ZprFile& file, const std::string& filename);

ZResult<ZprFile> readZPRFile(const std::string& filename, size_t maxDeltas);

std::vector<uint8_t> compressData(const std::vector<uint8_t>& inputData);

std::vector<uint8_t> decompressData(const std::vector<uint8_t>& compressedData);