#pragma once

#include <utility>
#include <zvcr/common/data_storage.hpp>
#include <zvcr/region/segment/segment_info.hpp>
#include <zvcr/common/generic_region.hpp>

namespace zvcr::region::dim3::segment3 {

    using namespace segment::segment_info;
    using namespace common::reverse_delta;
    using namespace segment::tile_entities;
    using common::generic_region::GenericRegion;

    using Sections3d = std::vector<DeltaBlockStates>;

    class Segment3d {
    public:
        Sections3d sections;
        SegmentInfo info;

        explicit Segment3d(Sections3d sections, SegmentInfo info): sections(std::move(sections)), info(std::move(info)) {}

        [[nodiscard]]
        DeltaBlockStates getSection(uint8_t y) const;

        [[nodiscard]]
        BlockStatesSnapshots snapshotFrom(time_t timestamp) const;

        [[nodiscard]]
        BlockStatesSnapshots latestSnapshot() const;

        [[nodiscard]]
        size_t updateSections(const BlockStatesSnapshots& sectionUpdates);
    };

    using Region3d = GenericRegion<Segment3d>;
    using Segments3d = Region3d::Segments;

    [[nodiscard]]
    Sections3d snapshotsToSections(const BlockStatesSnapshots& snapshots);

}
