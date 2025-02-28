#pragma once

#include <utility>
#include <zvcr/region/segment/segment_info.hpp>
#include <zvcr/region/dim2/layer.hpp>
#include <zvcr/common/generic_region.hpp>

namespace zvcr::region::dim2::segment2 {

    using namespace segment::segment_info;
    using namespace layer;
    using namespace common::definitions;
    using common::generic_region::GenericRegion;
    using common::result::OptionRef;

    class Segment2d {
    public:
        LayerContainer2d layers;
        SegmentInfo info;

        explicit Segment2d(LayerContainer2d layers, SegmentInfo segmentInfo): layers(std::move(layers)), info(std::move(segmentInfo)) {}
        Segment2d(): layers(LayerContainer2d {}), info(SegmentInfo {}) {}

        [[nodiscard]]
        OptionRef<const Layer2d> getLayer(LayerTypeId type) const;

        [[nodiscard]]
        OptionRef<const Layer2d> getLayer(LayerType layerType) const;

        [[nodiscard]]
        bool setLayer(LayerTypeId type, const Layer2d& layer);

        [[nodiscard]]
        bool setLayer(LayerType layerType, const Layer2d& layer);

        [[nodiscard]]
        bool setLayer(LayerTypeId type, const PackedSnapshot<Segment2dAtom>& initialState);

        [[nodiscard]]
        bool setLayer(LayerType layerType, const PackedSnapshot<Segment2dAtom>& initialState);

        [[nodiscard]]
        bool setLayer(LayerTypeId type, const std::vector<PackedSnapshot<Segment2dAtom>>& reverseDeltas, size_t snapshotLength);

        [[nodiscard]]
        bool setLayer(LayerType layerType, const std::vector<PackedSnapshot<Segment2dAtom>>& reverseDeltas, size_t snapshotLength);
    };

    using Region2d = GenericRegion<Segment2d>;
    using Segments2d = Region2d::Segments;

}
