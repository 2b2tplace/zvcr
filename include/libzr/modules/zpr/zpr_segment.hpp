#pragma once

#include <libzr/modules/common/zr_region.hpp>
#include <libzr/modules/common/zr_sector.hpp>
#include <libzr/modules/zpr/zpr_layer.hpp>

class ZprSegment : public ZrSector {
public:
    explicit ZprSegment(const ZprLayers& layers, const ZrChunkState& initialState, const ZrTileEntityCounts& initialTileEntityCounts);
    explicit ZprSegment(const ZprLayers& layers, const ChunkStates& chunkStates, const TileEntities& tileEntities);

    std::optional<ZprLayer> getLayer(uint8_t type) const;
    std::optional<ZprLayer> getLayer(ZprLayerType layerType) const;
    bool setLayer(uint8_t type, const ZprLayer& layer);
    bool setLayer(ZprLayerType layerType, const ZprLayer& layer);

    ZprLayers layers;
};

typedef ZrRegion<ZprSegment> ZprRegion;
typedef ZprRegion::Segments Segments;