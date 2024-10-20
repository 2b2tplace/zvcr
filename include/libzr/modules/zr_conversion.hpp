#pragma once

#include <libzr/modules/zpr/zpr_segment.hpp>
#include <libzr/modules/zvr/zvr_chunk.hpp>
#include <libzr/modules/common/zr_dimension.hpp>
#include <libzr/modules/zpr/zpr.hpp>
#include <libzr/modules/zvr/zvr.hpp>

class TileViewDeltas {
public:
    TileViewDeltas() = default;

    void emplaceMissingView(uint8_t layerType, time_t timestamp);
    ZrBlockStatesView& deltaView(ZprLayerType layerType, time_t timestamp);
    ZrBlockStatesView& deltaView(uint8_t layerType, time_t timestamp);
    ZrBlockStatesView& topDown(time_t timestamp);
    ZrBlockStatesView& roofless(time_t timestamp);
    ZrBlockStatesView& heightmap(time_t timestamp);
    ZrBlockStatesView& heightmapRoofless(time_t timestamp);
    ZrBlockStatesView& drainedTopDown(time_t timestamp);
    ZrBlockStatesView& drainedTopDownHeightmap(time_t timestamp);
    ZprLayers createLayers() const;
private:
    std::unordered_map<uint8_t, std::unordered_map<time_t, ZrBlockStatesView>> viewDeltas;
};

ZprFile convertZvrFileToZprFile(const ZvrFile& zvrFile);
ZprRegion convertZvrRegionToZprRegion(const ZvrRegion& zvrRegion, const ZvrDimensionProperties& properties);
std::optional<ZprSegment> convertZvrOptChunkToZprOptSegment(const std::optional<ZvrChunk>& zvrChunkOpt, const ZvrDimensionProperties& properties);
ZprSegment convertZvrChunkToZprSegment(const ZvrChunk& zvrChunk, const ZvrDimensionProperties& properties);
bool renderZprSegmentForSectionSnapshot(time_t timestamp, uint8_t sy, const ZrBlockStatesView& sectionView, TileViewDeltas& tileViewDeltas);