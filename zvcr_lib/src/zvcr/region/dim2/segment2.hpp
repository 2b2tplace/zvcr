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
        auto getLayer(LayerTypeId type) const -> result::OptionCRef<Layer2d<snapshotLength>> {
            if (!layers.contains(type)) return result::None;
            return layers.at(type);
        }

        [[nodiscard]]
        auto getLayer(LayerType layerType) const -> result::OptionCRef<Layer2d<snapshotLength>> {
            return getLayer(static_cast<LayerTypeId>(layerType));
        }

        [[nodiscard]]
        auto getLayer(LayerTypeId type) -> result::OptionRef<Layer2d<snapshotLength>> {
            if (!layers.contains(type)) return result::None;
            return layers.at(type);
        }

        [[nodiscard]]
        auto getLayer(LayerType layerType) -> result::OptionRef<Layer2d<snapshotLength>> {
            return getLayer(static_cast<LayerTypeId>(layerType));
        }

        auto setLayer(LayerTypeId type, const Layer2d<snapshotLength>& layer) -> void {
            layers[type] = layer;
        }

        auto setLayer(LayerType layerType, const Layer2d<snapshotLength>& layer) -> void {
            setLayer(static_cast<LayerTypeId>(layerType), layer);
        }

        auto setLayer(LayerTypeId type, const PackedSnapshot<snapshotLength>& initialState) -> void {
            setLayer(type, Layer2d(PackedDeltaData { initialState }, type));
        }

        auto setLayer(LayerType layerType, const PackedSnapshot<snapshotLength>& initialState) -> void {
            setLayer(static_cast<LayerTypeId>(layerType), initialState);
        }

        auto setLayer(LayerTypeId type, const std::vector<PackedSnapshot<snapshotLength>>& reverseDeltas) -> void {
            setLayer(type, Layer2d(PackedDeltaData { reverseDeltas }, type));
        }

        auto setLayer(LayerType layerType, const std::vector<PackedSnapshot<snapshotLength>>& reverseDeltas) -> void {
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
