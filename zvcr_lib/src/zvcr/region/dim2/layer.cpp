#include <zvcr/region/dim2/layer.hpp>

namespace zvcr {

    auto getHeightmapLayer(const LayerType topDownLayer) -> LayerType {
        switch (topDownLayer) {
            case LayerType::NORMAL:
                return LayerType::HEIGHTMAP;
            case LayerType::DRAINED:
                return LayerType::DRAINED_HEIGHTMAP;
            case LayerType::TERRAIN:
                return LayerType::TERRAIN_HEIGHTMAP;
            case LayerType::DRAINED_TERRAIN:
                return LayerType::DRAINED_TERRAIN_HEIGHTMAP;
            default:
                return LayerType::CUSTOM;
        }
    }

}
