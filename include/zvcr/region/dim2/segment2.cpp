#include <zvcr/region/dim2/segment2.hpp>

namespace zvcr::region::dim2::segment2 {

    OptionRef<const Layer2d> Segment2d::getLayer(const LayerTypeId type) const {
        if (!layers.contains(type)) return {};
        return {layers.at(type)};
    }

    OptionRef<const Layer2d> Segment2d::getLayer(LayerType layerType) const {
        if (layerType == LayerType::CUSTOM) return {};

        return getLayer(static_cast<LayerTypeId>(layerType));
    }

    bool Segment2d::setLayer(const LayerTypeId type, const Layer2d& layer) {
        layers[type] = layer;
        return true;
    }

    bool Segment2d::setLayer(const LayerType layerType, const Layer2d& layer) {
        if (layerType == LayerType::CUSTOM) return false;

        return setLayer(static_cast<LayerTypeId>(layerType), layer);
    }

    bool Segment2d::setLayer(const LayerTypeId type, const PackedSnapshot<Segment2dAtom>& initialState) {
        return setLayer(type, Layer2d(PackedDeltaData { initialState }, type));
    }

    bool Segment2d::setLayer(const LayerType layerType, const PackedSnapshot<Segment2dAtom>& initialState) {
        if (layerType == LayerType::CUSTOM) return false;

        return setLayer(static_cast<LayerTypeId>(layerType), initialState);
    }

    bool Segment2d::setLayer(const LayerTypeId type, const std::vector<PackedSnapshot<Segment2dAtom>>& reverseDeltas,
                             const size_t snapshotLength) {
        return setLayer(type, Layer2d(PackedDeltaData { reverseDeltas, snapshotLength }, type));
    }

    bool Segment2d::setLayer(const LayerType layerType, const std::vector<PackedSnapshot<Segment2dAtom>>& reverseDeltas,
                             const size_t snapshotLength) {
        if (layerType == LayerType::CUSTOM) return false;

        return setLayer(static_cast<LayerTypeId>(layerType), reverseDeltas, snapshotLength);
    }

}