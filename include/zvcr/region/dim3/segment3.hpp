#pragma once

#include <zvcr/common/data_storage.hpp>
#include <zvcr/region/segment/segment_info.hpp>
#include <zvcr/common/generic_region.hpp>

namespace zvcr::region::dim3::segment3 {

    using namespace segment::segment_info;
    using namespace common::reverse_delta;
    using namespace segment::tile_entities;
    using namespace common::definitions;
    using common::generic_region::GenericRegion;

    template<typename T>
    class DeltaSections3d {
        using Segment3dSnapshot = std::vector<PackedSnapshot<T>>;
        using PackedData = PackedDeltaData<T>;

        std::vector<PackedData> sections;

    public:
        size_t sectionCount;
        size_t sectionSize;

        explicit DeltaSections3d(const size_t sectionCount, const size_t sectionSize):
            sectionCount(sectionCount), sectionSize(sectionSize) {
            sections.resize(sectionCount, PackedData(sectionSize));
        }

        [[nodiscard]]
        PackedData& getSection(uint8_t y) {
            return sections[y];
        }

        [[nodiscard]]
        const PackedData& readSection(uint8_t y) const {
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
        Segment3dSnapshot snapshotFrom(time_t timestamp) const {
            Segment3dSnapshot snapshots{};
            snapshots.reserve(sectionCount);

            for (const auto& section : this->sections) {
                if (const auto snapshot = section.snapshotFrom(timestamp); snapshot.hasSome())
                    snapshots.push_back(snapshot.unwrap());
            }
            return snapshots;
        }

        [[nodiscard]]
        Segment3dSnapshot latestSnapshot() const {
            Segment3dSnapshot snapshots{};
            snapshots.reserve(sectionCount);

            for (const auto& section : this->sections) {
                if (const auto snapshot = section.latestSnapshot(); snapshot.hasSome())
                    snapshots.push_back(snapshot.unwrap());
            }
            return snapshots;
        }

        [[nodiscard]]
        size_t updateSections(const Segment3dSnapshot& sectionUpdates) {
            size_t changes = 0;
            for (size_t section = 0; section < sectionUpdates.size(); ++section)
                changes += sections[section].insertSnapshot(sectionUpdates[section]).orElse(0);

            return changes;
        }
    };

    using BlockSections3d = DeltaSections3d<BlockStateId>;
    using BiomeSections3d = DeltaSections3d<BiomeId>;

    struct Segment3d {
        size_t sectionCount;
        BlockSections3d blockSections;
        BiomeSections3d biomeSections;
        SegmentInfo info;

        explicit Segment3d(const size_t sectionCount):
            sectionCount(sectionCount),
            blockSections(BlockSections3d {sectionCount, SECTION_3D_SIZE_BLOCKS}),
            biomeSections(BiomeSections3d {sectionCount, SECTION_3D_SIZE_BIOMES}),
            info({}) {}
    };

    using Region3d = GenericRegion<Segment3d>;
    using Segments3d = Region3d::Segments;

}
