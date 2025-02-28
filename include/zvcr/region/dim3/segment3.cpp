#include <zvcr/region/dim3/segment3.hpp>

namespace zvcr::region::dim3::segment3 {

    PackedBlockData Segment3d::getSection(const uint8_t y) const {
        return sections[y];
    }

    Segment3dSnapshot Segment3d::snapshotFrom(const time_t timestamp) const {
        Segment3dSnapshot snapshots{};
        snapshots.reserve(this->sections.size());

        for (const auto& section : this->sections) {
            if (const auto snapshot = section.snapshotFrom(timestamp); snapshot.hasSome())
                snapshots.push_back(snapshot.unwrap());
        }

        return snapshots;
    }

    Segment3dSnapshot Segment3d::latestSnapshot() const {
        Segment3dSnapshot snapshots{};
        snapshots.reserve(this->sections.size());

        for (const auto& section : this->sections) {
            if (const auto snapshot = section.latestSnapshot(); snapshot.hasSome())
                snapshots.push_back(snapshot.unwrap());
        }

        return snapshots;
    }

    size_t Segment3d::updateSections(const Segment3dSnapshot& sectionUpdates) {
        size_t changes = 0;
        for (size_t section = 0; section < sectionUpdates.size(); ++section)
            changes += sections[section].insertSnapshot(sectionUpdates[section]).orElse(0);

        return changes;
    }

    Sections3d snapshotsToSections3d(const Segment3dSnapshot& snapshots) {
        auto sections = std::vector<PackedBlockData>{};
        sections.reserve(snapshots.size());

        for (const auto& snapshot : snapshots)
            sections.emplace_back(snapshot);

        return sections;
    }
}