#include <zvcr/region/dim2/layer.hpp>
#include <zvcr/common/definitions.hpp>

namespace zvcr::region::dim2::layer {

    Layer2d& LayerTable2d::operator[](const LayerType layerType) {
        return operator[](static_cast<LayerTypeId>(layerType));
    }

    Layer2d& LayerTable2d::operator[](const LayerTypeId layerType) {
        if (contains(layerType)) return at(layerType);

        const auto deltas = PackedDeltaData(std::vector<PackedSnapshot<Segment2dAtom>>{}, snapshotSize);
        const auto emptyLayer = Layer2d{deltas, layerType};
        emplace(layerType, emptyLayer);

        return at(layerType);
    }

}
