#pragma once

#include <libzr/modules/common/zr_delta.hpp>

enum class ZprLayerType {
    TOP_DOWN = 0,
    TOP_DOWN_ROOFLESS = 1,
    HEIGHTMAP = 2,
    HEIGHTMAP_ROOFLESS = 3,
    DRAINED_TOP_DOWN = 4,
    DRAINED_TOP_DOWN_HEIGHTMAP = 5,
    CUSTOM
};

struct ZprLayer {
    ZrDeltaBlockStates deltas;
    uint8_t type{};
};

class ZprLayers : public std::unordered_map<uint8_t, ZprLayer> {
public:
    ZprLayer& operator[](ZprLayerType layerType);
    ZprLayer& operator[](uint8_t layerType);
};