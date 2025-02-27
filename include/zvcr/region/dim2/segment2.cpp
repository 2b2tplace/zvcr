#include <zvcr/region/dim2/segment2.hpp>

namespace zvcr::region::dim2::segment2 {

    OptionRef<const Layer2d> Segment2d::getLayer(const uint8_t type) const {
        if (!layers.contains(type)) return {};
        return {layers.at(type)};
    }

    OptionRef<const Layer2d> Segment2d::getLayer(LayerType layerType) const {
        if (layerType == LayerType::CUSTOM) return {};

        return getLayer(static_cast<uint8_t>(layerType));
    }

    bool Segment2d::setLayer(const uint8_t type, const Layer2d& layer) {
        layers[type] = layer;
        return true;
    }

    bool Segment2d::setLayer(const LayerType layerType, const Layer2d& layer) {
        if (layerType == LayerType::CUSTOM) return false;

        return setLayer(static_cast<uint8_t>(layerType), layer);
    }

    bool Segment2d::setLayer(const uint8_t type, const BlockStatesSnapshot& initialState) {
        return setLayer(type, Layer2d { DeltaBlockStates { initialState }, type});
    }

    bool Segment2d::setLayer(const LayerType layerType, const BlockStatesSnapshot& initialState) {
        if (layerType == LayerType::CUSTOM) return false;

        return setLayer(static_cast<uint8_t>(layerType), initialState);
    }

    bool Segment2d::setLayer(const uint8_t type, const std::vector<BlockStatesSnapshot>& reverseDeltas,
                             const size_t snapshotLength) {
        return setLayer(type, Layer2d { DeltaBlockStates { reverseDeltas, snapshotLength }, type});
    }

    bool Segment2d::setLayer(const LayerType layerType, const std::vector<BlockStatesSnapshot>& reverseDeltas,
                             const size_t snapshotLength) {
        if (layerType == LayerType::CUSTOM) return false;

        return setLayer(static_cast<uint8_t>(layerType), reverseDeltas, snapshotLength);
    }

}