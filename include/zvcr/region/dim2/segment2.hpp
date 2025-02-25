#pragma once

#include <optional>
#include <utility>
#include <zvcr/region/segment/segment_info.hpp>
#include <zvcr/region/dim2/layer.hpp>
#include <zvcr/common/generic_region.hpp>

namespace zvcr::region::dim2::segment2 {

    using namespace segment::segment_info;
    using namespace layer;
    using common::generic_region::GenericRegion;

    class Segment2d {
    public:
        Layers2d layers;
        SegmentInfo info;

        explicit Segment2d(Layers2d layers, SegmentInfo segmentInfo): layers(std::move(layers)), info(std::move(segmentInfo)) {}
        Segment2d(): layers(Layers2d {}), info(SegmentInfo {}) {}

        [[nodiscard]]
        std::optional<Layer2d> getLayer(uint8_t type) const;

        [[nodiscard]]
        std::optional<Layer2d> getLayer(LayerType layerType) const;

        [[nodiscard]]
        bool setLayer(uint8_t type, const Layer2d& layer);

        [[nodiscard]]
        bool setLayer(LayerType layerType, const Layer2d& layer);
    };

    using Region2d = GenericRegion<Segment2d>;
    using Segments2d = Region2d::Segments;

}
