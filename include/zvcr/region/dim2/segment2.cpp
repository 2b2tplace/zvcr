#include <zvcr/region/dim2/segment2.hpp>

namespace zvcr::region::dim2::segment2 {

    std::optional<Layer2d> Segment2d::getLayer(const uint8_t type) const {
        if (!layers.contains(type)) return std::nullopt;
        return layers.at(type);
    }

    std::optional<Layer2d> Segment2d::getLayer(LayerType layerType) const {
        if (layerType == LayerType::CUSTOM) return std::nullopt;

        return getLayer(static_cast<uint8_t>(layerType));
    }

    bool Segment2d::setLayer(const uint8_t type, const Layer2d &layer) {
        layers[type] = layer;
        return true;
    }

    bool Segment2d::setLayer(const LayerType layerType, const Layer2d &layer) {
        if (layerType == LayerType::CUSTOM) return false;

        return setLayer(static_cast<uint8_t>(layerType), layer);
    }

}