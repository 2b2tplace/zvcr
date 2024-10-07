#include <libzr/modules/zpr/zpr_layer.hpp>

ZprLayer& ZprLayers::operator[](const ZprLayerType layerType) {
    return operator[](static_cast<uint8_t>(layerType));
}

ZprLayer& ZprLayers::operator[](const uint8_t layerType) {
    if (contains(layerType)) return at(layerType);

    const auto deltas = ZrDeltaBlockStates(std::vector<ZrBlockStatesSnapshot>(), TILE_SIZE);
    const auto emptyLayer = ZprLayer{deltas, layerType};
    emplace(layerType, emptyLayer);

    return at(layerType);
}