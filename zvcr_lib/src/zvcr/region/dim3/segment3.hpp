#pragma once

#include <zvcr/common/data_storage.hpp>
#include <zvcr/region/segment/segment_info.hpp>
#include <zvcr/common/generic_region.hpp>
#include <zvcr/region/dimension.hpp>

namespace zvcr {

    static constexpr size_t MAX_SECTION_COUNT = 24;

    template<size_t snapshotLength>
    class DeltaSections3d {
        using Segment3dSnapshot = std::vector<UnpackedData<snapshotLength>>;
        using Segment3dPackedSnapshot = std::vector<PackedSnapshot<snapshotLength>>;
        using PackedData = PackedDeltaData<snapshotLength>;

    public:
        std::array<PackedData, MAX_SECTION_COUNT> sections{};
        size_t sectionCount;

        explicit DeltaSections3d(const size_t sectionCount): sectionCount(sectionCount) {}

        [[nodiscard]]
        result::Option<Segment3dSnapshot> snapshotFrom(const time_t timestamp) const {
            Segment3dSnapshot snapshots;
            snapshots.reserve(sectionCount);

            for (const auto& section : this->sections) {
                const auto snapshot = section.snapshotFrom(timestamp);
                if (!snapshot) return result::None;

                snapshots.push_back(*snapshot);
            }
            return snapshots;
        }

        [[nodiscard]]
        result::Option<Segment3dSnapshot> latestSnapshot(time_t *getEarliestTimestamp = nullptr) const {
            Segment3dSnapshot snapshots;
            snapshots.reserve(sectionCount);

            for (const auto& section : this->sections) {
                const auto snapshot = section.latestSnapshot();
                if (!snapshot) return result::None;

                if (const auto timestamp = snapshot->get().timestamp; getEarliestTimestamp && timestamp < *getEarliestTimestamp)
                    *getEarliestTimestamp = timestamp;

                snapshots.push_back(snapshot->get().data.unpack());
            }
            return snapshots;
        }

        [[nodiscard]]
        size_t updateSections(const Segment3dPackedSnapshot& sectionUpdates) {
            size_t changes = 0;
            for (size_t section = 0; section < sectionUpdates.size(); ++section)
                changes += sections[section].insertSnapshot(sectionUpdates[section]).value_or(0);

            return changes;
        }
    };

    struct Segment3d {
        size_t sectionCount;
        DeltaSections3d<SECTION_3D_SIZE_BLOCKS> blockSections;
        DeltaSections3d<SECTION_3D_SIZE_BIOMES> biomeSections;
        SegmentInfo info;
        bool supportBiomes;

        explicit Segment3d(const DimensionType dimension, const bool supportBiomes):
            Segment3d(getProperties(dimension), supportBiomes) {}

        explicit Segment3d(const DimensionProperties& dimensionProperties, const bool supportBiomes):
            Segment3d(dimensionProperties.height / SEGMENT_SIDELENGTH_BLOCKS, supportBiomes) {}

        explicit Segment3d(const size_t sectionCount, const bool supportBiomes):
            sectionCount(sectionCount),
            blockSections(sectionCount),
            biomeSections(sectionCount),
            info(SegmentStates{}),
            supportBiomes(supportBiomes) {}
    };

    using Region3d = GenericRegion<Segment3d>;
    using Segments3d = Region3d::Segments;

}
