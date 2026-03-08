#pragma once

#include <zvcr/region/segment/segment_info.hpp>
#include <zvcr/region/dim2/layer.hpp>
#include <zvcr/common/generic_region.hpp>

namespace zvcr {

    template<size_t snapshotLength>
    class LayerContainer2d {
    public:
        LayerTable2d<snapshotLength> layers;

        explicit LayerContainer2d(LayerTable2d<snapshotLength> layers):
            layers(std::move(layers)) {}

        explicit LayerContainer2d():
            layers(LayerTable2d<snapshotLength>{}) {}

        [[nodiscard]]
        result::OptionCRef<Layer2d<snapshotLength>> getLayer(LayerTypeId type) const {
            if (!layers.contains(type)) return result::None;
            return layers.at(type);
        }

        [[nodiscard]]
        result::OptionCRef<Layer2d<snapshotLength>> getLayer(LayerType layerType) const {
            return getLayer(static_cast<LayerTypeId>(layerType));
        }

        [[nodiscard]]
        result::OptionRef<Layer2d<snapshotLength>> getLayer(LayerTypeId type) {
            if (!layers.contains(type)) return result::None;
            return layers.at(type);
        }

        [[nodiscard]]
        result::OptionRef<Layer2d<snapshotLength>> getLayer(LayerType layerType) {
            return getLayer(static_cast<LayerTypeId>(layerType));
        }

        void setLayer(LayerTypeId type, const Layer2d<snapshotLength>& layer) {
            layers[type] = layer;
        }

        void setLayer(LayerType layerType, const Layer2d<snapshotLength>& layer) {
            setLayer(static_cast<LayerTypeId>(layerType), layer);
        }

        void setLayer(LayerTypeId type, const PackedSnapshot<snapshotLength>& initialState) {
            setLayer(type, Layer2d(PackedDeltaData { initialState }, type));
        }

        void setLayer(LayerType layerType, const PackedSnapshot<snapshotLength>& initialState) {
            setLayer(static_cast<LayerTypeId>(layerType), initialState);
        }

        void setLayer(LayerTypeId type, const std::vector<PackedSnapshot<snapshotLength>>& reverseDeltas) {
            setLayer(type, Layer2d(PackedDeltaData { reverseDeltas }, type));
        }

        void setLayer(LayerType layerType, const std::vector<PackedSnapshot<snapshotLength>>& reverseDeltas) {
            setLayer(static_cast<LayerTypeId>(layerType), reverseDeltas);
        }
    };

    struct Segment2d {
        LayerContainer2d<SECTION_2D_SIZE_BLOCKS> layers{};
        LayerContainer2d<SECTION_2D_SIZE_BIOMES> biomeLayers{};
        SegmentInfo info;
        bool supportBiomes;

        explicit Segment2d(const bool supportBiomes = true):
            info(SegmentStates{}),
            supportBiomes(supportBiomes) {}
    };

    using Region2d = GenericRegion<Segment2d>;
    using Segments2d = Region2d::Segments;

}
