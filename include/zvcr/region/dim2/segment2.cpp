#include <zvcr/region/dim2/segment2.hpp>

namespace zvcr::region::dim2::segment2 {

    OptionCRef<Layer2d> LayerContainer2d::getLayer(const LayerTypeId type) const {
        if (!layers.contains(type)) return {};
        return {layers.at(type)};
    }

    OptionCRef<Layer2d> LayerContainer2d::getLayer(LayerType layerType) const {
        return getLayer(static_cast<LayerTypeId>(layerType));
    }

    void LayerContainer2d::setLayer(const LayerTypeId type, const Layer2d& layer) {
        layers[type] = layer;
    }

    void LayerContainer2d::setLayer(const LayerType layerType, const Layer2d& layer) {
        setLayer(static_cast<LayerTypeId>(layerType), layer);
    }

    void LayerContainer2d::setLayer(const LayerTypeId type, const PackedSnapshot<SegmentAtom>& initialState) {
        setLayer(type, Layer2d(PackedDeltaData { initialState }, type));
    }

    void LayerContainer2d::setLayer(const LayerType layerType, const PackedSnapshot<SegmentAtom>& initialState) {
        setLayer(static_cast<LayerTypeId>(layerType), initialState);
    }

    void LayerContainer2d::setLayer(const LayerTypeId type, const std::vector<PackedSnapshot<SegmentAtom>>& reverseDeltas,
                             const size_t snapshotLength) {
        setLayer(type, Layer2d(PackedDeltaData { reverseDeltas, snapshotLength }, type));
    }

    void LayerContainer2d::setLayer(const LayerType layerType, const std::vector<PackedSnapshot<SegmentAtom>>& reverseDeltas,
                             const size_t snapshotLength) {
        setLayer(static_cast<LayerTypeId>(layerType), reverseDeltas, snapshotLength);
    }

}