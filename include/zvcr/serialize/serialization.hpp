#pragma once

#include <functional>
#include <optional>
#include <zvcr/common/result.hpp>
#include <zvcr/common/paletted_storage.hpp>
#include <zvcr/common/reverse_delta.hpp>
#include <zvcr/region/dimension.hpp>
#include <zvcr/region/segment/segment_info.hpp>
#include <zvcr/region/dim3/segment3.hpp>
#include <zvcr/region/dim3/zvcr3.hpp>
#include <zvcr/region/dim2/segment2.hpp>
#include <zvcr/region/dim2/zvcr2.hpp>
#include <zvcr/region/dim2/layer.hpp>
#include <zvcr/common/definitions.hpp>

namespace zvcr::serialize::serialization {

    using namespace common::result;
    using namespace common::reverse_delta;
    using namespace region::segment::segment_info;
    using namespace region::dim3::segment3;
    using namespace region::dim3::zvcr3;
    using namespace region::dim2::segment2;
    using namespace region::dim2::zvcr2;
    using namespace region::dim2::layer;
    using namespace common::paletted_storage;
    using namespace zvcr::common::definitions;
    using namespace region::dimension;

    enum ZVCRError {
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
    using ZVCRResult = Result<R, ZVCRError>;

    template<typename R>
    using ZVCRFileSerialize = std::function<void(const R&, std::vector<uint8_t>&)>;

    template<typename R>
    using ZVCRFileDeserialize = std::function<ZVCRResult<R>(const std::vector<uint8_t>&, size_t&, size_t)>;

    ZVCRResult<DimensionType> deserializeDimensionType(const std::vector<uint8_t>& data, size_t& offset);

    template<typename Version>
    ZVCRResult<Version> deserializeVersion(const std::vector<uint8_t>& data, size_t& offset, Version latest);

    std::optional<ZVCRError> validateZVCRFilePrefix(const std::vector<uint8_t>& data, size_t& offset, const std::string& prefix);

    template<typename R>
    size_t writeZVCRFile(const R& file, const std::string& filename, const ZVCRFileSerialize<R>& serialize, int zstdCompressionLevel, int zstdCompressionThreads);

    template<typename R>
    ZVCRResult<R> readZVCRFile(const std::string& filename, size_t maxDeltas, const ZVCRFileDeserialize<R>& deserialize);

    void serializeBlockStatesSnapshot(const BlockStatesSnapshot& snapshot, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

    ZVCRResult<BlockStatesSnapshot> deserializeBlockStatesSnapshot(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t snapshotLength);

    void serializePaletteTable(const std::vector<Palette>& paletteTable, std::vector<uint8_t>& data);

    ZVCRResult<std::vector<Palette>> deserializePaletteTable(const std::vector<uint8_t>& data, size_t& offset);

    std::optional<ZVCRError> skipBlockStatesSnapshot(const std::vector<uint8_t>& data, size_t& offset);

    void serializeBlockStates(const DeltaBlockStates& section3d, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

    ZVCRResult<DeltaBlockStates> deserializeBlockStates(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t maxDeltas, size_t snapshotLength);

    void serializeSegmentState(const SegmentState& segmentState, std::vector<uint8_t>& data);

    ZVCRResult<SegmentState> deserializeSegmentState(const std::vector<uint8_t>& data, size_t& offset);

    void serializeTileEntityCountInfo(const TileEntityCountInfo& tileEntityCounts, std::vector<uint8_t>& data);

    ZVCRResult<TileEntityCountInfo> deserializeTileEntityCountInfo(const std::vector<uint8_t>& data, size_t& offset);

    void serializeSegmentInfo(const SegmentInfo& segmentInfo, std::vector<uint8_t>& data);

    ZVCRResult<SegmentInfo> deserializeSegmentInfo(const std::vector<uint8_t>& data, size_t& offset);

    void serializeSegment3d(const Segment3d& segment3d, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

    ZVCRResult<Segment3d> deserializeSegment3d(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t maxDeltas, uint32_t sectionAmount);

    void serializeOptSegment3d(const std::optional<Segment3d>& segment3dOpt, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

    ZVCRResult<std::optional<Segment3d>> deserializeOptSegment3d(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t maxDeltas, uint32_t sectionAmount);

    void serializeRegion3d(const Region3d& region, std::vector<uint8_t>& data);

    ZVCRResult<Region3d> deserializeRegion3d(const std::vector<uint8_t>& data, size_t& offset, size_t maxDeltas, uint32_t sectionAmount);

    void serializeZVCR3File(const ZVCR3File& file, std::vector<uint8_t>& data);

    ZVCRResult<ZVCR3File> deserializeZVCR3File(const std::vector<uint8_t>& data, size_t& offset, size_t maxDeltas);

    size_t writeZVCR3File(const ZVCR3File& file, const std::string& filename, int zstdCompressionLevel, int zstdCompressionThreads);

    ZVCRResult<ZVCR3File> readZVCR3File(const std::string& filename, size_t maxDeltas);

    void serializeLayer(const Layer2d& layer, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

    ZVCRResult<Layer2d> deserializeLayer(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t maxDeltas);

    void serializeLayers(const Layers2d& layers, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

    ZVCRResult<Layers2d> deserializeLayers(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t maxDeltas);

    void serializeSegment2d(const Segment2d& segment, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

    ZVCRResult<Segment2d> deserializeSegment2d(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t maxDeltas);

    void serializeOptSegment2d(const std::optional<Segment2d>& segment, std::vector<uint8_t>& data, std::vector<Palette>& paletteTable);

    ZVCRResult<std::optional<Segment2d>> deserializeOptSegment2d(const std::vector<uint8_t>& data, size_t& offset, const std::vector<Palette>& paletteTable, size_t maxDeltas);

    void serializeRegion2d(const Region2d& region, std::vector<uint8_t>& data);

    ZVCRResult<Region2d> deserializeRegion2d(const std::vector<uint8_t>& data, size_t& offset, size_t maxDeltas);

    void serializeZVCR2File(const ZVCR2File& file, std::vector<uint8_t>& data);

    ZVCRResult<ZVCR2File> deserializeZVCR2File(const std::vector<uint8_t>& data, size_t& offset, size_t maxDeltas);

    size_t writeZVCR2File(const ZVCR2File& file, const std::string& filename, int zstdCompressionLevel, int zstdCompressionThreads);

    ZVCRResult<ZVCR2File> readZVCR2File(const std::string& filename, size_t maxDeltas);

}
