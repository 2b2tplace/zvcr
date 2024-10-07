#include <libzr/modules/zpr/zpr_segment.hpp>

ZprSegment::ZprSegment(const ZprLayers& layers, const ZrChunkState& initialState, const ZrTileEntityCounts& initialTileEntityCounts): ZrSector(initialState, initialTileEntityCounts) {
    this->layers = layers;
}

ZprSegment::ZprSegment(const ZprLayers& layers, const ChunkStates& chunkStates, const TileEntities& tileEntities): ZrSector(chunkStates, tileEntities) {
    this->layers = layers;
}

std::optional<ZprLayer> ZprSegment::getLayer(const uint8_t type) const {
    if (type >= layers.size()) return std::nullopt;

    return layers.at(type);
}

std::optional<ZprLayer> ZprSegment::getLayer(ZprLayerType layerType) const {
    if (layerType == ZprLayerType::CUSTOM) return std::nullopt;

    return getLayer(static_cast<uint8_t>(layerType));
}

bool ZprSegment::setLayer(const uint8_t type, const ZprLayer &layer) {
    if (type >= layers.size()) return false;

    layers[type] = layer;
    return true;
}

bool ZprSegment::setLayer(const ZprLayerType layerType, const ZprLayer &layer) {
    if (layerType == ZprLayerType::CUSTOM) return false;

    return setLayer(static_cast<uint8_t>(layerType), layer);
}
