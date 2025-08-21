#include <zvcr/region/dim2/layer.hpp>
#include <zvcr/common/definitions.hpp>

namespace zvcr {

    LayerType getHeightmapLayer(const LayerType topDownLayer) {
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


    Layer2d& LayerTable2d::operator[](const LayerType layerType) {
        return operator[](static_cast<LayerTypeId>(layerType));
    }

    Layer2d& LayerTable2d::operator[](const LayerTypeId layerType) {
        if (contains(layerType)) return at(layerType);

        const auto deltas = PackedDeltaData{std::vector<PackedSnapshot>{}, snapshotSize};
        const auto emptyLayer = Layer2d{deltas, layerType};
        emplace(layerType, emptyLayer);

        return at(layerType);
    }

}
