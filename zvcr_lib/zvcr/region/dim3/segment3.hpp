#pragma once

#include <zvcr/common/data_storage.hpp>
#include <zvcr/region/segment/segment_info.hpp>
#include <zvcr/common/generic_region.hpp>
#include <zvcr/region/dimension.hpp>

namespace zvcr {

    class DeltaSections3d {
        using Segment3dSnapshot = std::vector<PackedSnapshot>;
        using PackedData = PackedDeltaData;

        std::vector<PackedData> sections;

    public:
        size_t sectionCount;
        size_t sectionSize;

        explicit DeltaSections3d(const size_t sectionCount, const size_t sectionSize):
            sectionCount(sectionCount), sectionSize(sectionSize) {
            sections.resize(sectionCount, PackedData(sectionSize));
        }

        [[nodiscard]]
        PackedData& getSection(const uint8_t y) {
            return sections[y];
        }

        [[nodiscard]]
        const PackedData& readSection(const uint8_t y) const {
            return sections[y];
        }

        [[nodiscard]]
        const std::vector<PackedData>& getSections() const {
            return sections;
        }

        [[nodiscard]]
        size_t getSectionCount() const {
            return sectionCount;
        }

        [[nodiscard]]
        Segment3dSnapshot snapshotFrom(const time_t timestamp) const {
            Segment3dSnapshot snapshots;
            snapshots.reserve(sectionCount);

            for (const auto& section : this->sections) {
                if (const auto snapshot = section.snapshotFrom(timestamp); snapshot.has_value())
                    snapshots.push_back(snapshot.value());
            }
            return snapshots;
        }

        [[nodiscard]]
        Segment3dSnapshot latestSnapshot() const {
            Segment3dSnapshot snapshots;
            snapshots.reserve(sectionCount);

            for (const auto& section : this->sections) {
                if (const auto snapshot = section.latestSnapshot(); snapshot.has_value())
                    snapshots.push_back(snapshot.value());
            }
            return snapshots;
        }

        [[nodiscard]]
        size_t updateSections(const Segment3dSnapshot& sectionUpdates) {
            size_t changes = 0;
            for (size_t section = 0; section < sectionUpdates.size(); ++section)
                changes += sections[section].insertSnapshot(sectionUpdates[section]).value_or(0);

            return changes;
        }
    };

    struct Segment3d {
        size_t sectionCount;
        DeltaSections3d blockSections;
        DeltaSections3d biomeSections;
        SegmentInfo info;
        bool supportBiomes;

        explicit Segment3d(const DimensionType dimension, const bool supportBiomes):
            Segment3d(getProperties(dimension), supportBiomes) {}

        explicit Segment3d(const DimensionProperties& dimensionProperties, const bool supportBiomes):
            Segment3d(dimensionProperties.height / SEGMENT_SIDELENGTH_BLOCKS, supportBiomes) {}

        explicit Segment3d(const size_t sectionCount, const bool supportBiomes):
            sectionCount(sectionCount),
            blockSections(DeltaSections3d{sectionCount, SECTION_3D_SIZE_BLOCKS}),
            biomeSections(DeltaSections3d{sectionCount, SECTION_3D_SIZE_BIOMES}),
            info({}),
            supportBiomes(supportBiomes) {}
    };

    using Region3d = GenericRegion<Segment3d>;
    using Segments3d = Region3d::Segments;

}
