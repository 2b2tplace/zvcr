#pragma once

#include <utility>
#include <zvcr/region/segment/segment_info.hpp>
#include <zvcr/region/dim2/layer.hpp>
#include <zvcr/common/generic_region.hpp>

namespace zvcr::region {

    using namespace definitions;
    using generic_region::GenericRegion;
    using result::OptionRef;

    class LayerContainer2d {
    public:
        LayerTable2d layers;
        size_t snapshotSize;

        explicit LayerContainer2d(LayerTable2d layers):
            layers(std::move(layers)), snapshotSize(layers.snapshotSize) {
            for (const Layer2d& layer : layers | std::views::values)
                assert(layer.deltas.snapshotLength == layers.snapshotSize
                    && "Layer Table contains Layers with differing snapshot size");
        }

        explicit LayerContainer2d(const size_t snapshotSize):
            layers(LayerTable2d {snapshotSize}), snapshotSize(snapshotSize) {}

        [[nodiscard]]
        OptionCRef<Layer2d> getLayer(LayerTypeId type) const;

        [[nodiscard]]
        OptionCRef<Layer2d> getLayer(LayerType layerType) const;

        void setLayer(LayerTypeId type, const Layer2d& layer);

        void setLayer(LayerType layerType, const Layer2d& layer);

        void setLayer(LayerTypeId type, const PackedSnapshot<SegmentAtom>& initialState);

        void setLayer(LayerType layerType, const PackedSnapshot<SegmentAtom>& initialState);

        void setLayer(LayerTypeId type, const std::vector<PackedSnapshot<SegmentAtom>>& reverseDeltas, size_t snapshotLength);

        void setLayer(LayerType layerType, const std::vector<PackedSnapshot<SegmentAtom>>& reverseDeltas, size_t snapshotLength);
    };

    struct Segment2d {
        LayerContainer2d layers{SECTION_2D_SIZE_BLOCKS};
        LayerContainer2d biomeLayers{SECTION_2D_SIZE_BIOMES};
        SegmentInfo info;
    };

    using Region2d = GenericRegion<Segment2d>;
    using Segments2d = Region2d::Segments;

}
