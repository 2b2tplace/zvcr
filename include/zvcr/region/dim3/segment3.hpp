#pragma once

#include <utility>
#include <zvcr/common/data_storage.hpp>
#include <zvcr/region/segment/segment_info.hpp>
#include <zvcr/common/generic_region.hpp>

namespace zvcr::region::dim3::segment3 {

    using namespace segment::segment_info;
    using namespace common::reverse_delta;
    using namespace segment::tile_entities;
    using namespace common::definitions;
    using common::generic_region::GenericRegion;

    using PackedBlockData = PackedDeltaData<BlockStateId>;

    using Sections3d = std::vector<PackedBlockData>;
    using Segment3dSnapshot = std::vector<PackedSnapshot<BlockStateId>>;

    class Segment3d {
    public:
        Sections3d sections;
        SegmentInfo info;

        explicit Segment3d(Sections3d sections, SegmentInfo info): sections(std::move(sections)), info(std::move(info)) {}

        [[nodiscard]]
        PackedBlockData getSection(uint8_t y) const;

        [[nodiscard]]
        Segment3dSnapshot snapshotFrom(time_t timestamp) const;

        [[nodiscard]]
        Segment3dSnapshot latestSnapshot() const;

        [[nodiscard]]
        size_t updateSections(const Segment3dSnapshot& sectionUpdates);
    };

    using Region3d = GenericRegion<Segment3d>;
    using Segments3d = Region3d::Segments;

    [[nodiscard]]
    Sections3d snapshotsToSections(const Segment3dSnapshot& snapshots);

}
