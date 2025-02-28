#include <zvcr/region/dim2/layer.hpp>
#include <zvcr/common/definitions.hpp>

namespace zvcr::region::dim2::layer {

    using common::definitions::SECTION_2D_SIZE_BLOCKS;

    Layer2d& LayerContainer2d::operator[](const LayerType layerType) {
        return operator[](static_cast<LayerTypeId>(layerType));
    }

    Layer2d& LayerContainer2d::operator[](const LayerTypeId layerType) {
        if (contains(layerType)) return at(layerType);

        const auto deltas = PackedDeltaData(std::vector<PackedSnapshot<Segment2dAtom>>{}, SECTION_2D_SIZE_BLOCKS);
        const auto emptyLayer = Layer2d{deltas, layerType};
        emplace(layerType, emptyLayer);

        return at(layerType);
    }

}
