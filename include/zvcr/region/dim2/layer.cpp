#include <zvcr/region/dim2/layer.hpp>
#include <zvcr/common/definitions.hpp>

namespace zvcr::region::dim2::layer {

    using common::definitions::SECTION_2D_SIZE_BLOCKS;

    Layer2d& Layers2d::operator[](const LayerType layerType) {
        return operator[](static_cast<uint8_t>(layerType));
    }

    Layer2d& Layers2d::operator[](const uint8_t layerType) {
        if (contains(layerType)) return at(layerType);

        const auto deltas = DeltaBlockStates(std::vector<BlockStatesSnapshot>{}, SECTION_2D_SIZE_BLOCKS);
        const auto emptyLayer = Layer2d{deltas, layerType};
        emplace(layerType, emptyLayer);

        return at(layerType);
    }

}
