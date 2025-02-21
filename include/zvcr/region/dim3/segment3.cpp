#include <zvcr/region/dim3/segment3.hpp>

namespace zvcr::region::dim3::segment3 {

    DeltaBlockStates Segment3d::getSection(const uint8_t y) const {
        return sections[y];
    }

    BlockStatesSnapshots Segment3d::snapshotFrom(const time_t timestamp) const {
        BlockStatesSnapshots snapshots{};
        snapshots.reserve(this->sections.size());

        for (const auto & section : this->sections)
            snapshots.push_back(section.snapshotFrom(timestamp));

        return snapshots;
    }

    BlockStatesSnapshots Segment3d::latestSnapshot() const {
        BlockStatesSnapshots snapshots{};
        snapshots.reserve(this->sections.size());

        for (const auto & section : this->sections)
            snapshots.push_back(section.latestSnapshot());

        return snapshots;
    }

    size_t Segment3d::updateSections(const BlockStatesSnapshots &sectionUpdates) {
        size_t changes = 0;
        for (size_t section = 0; section < sectionUpdates.size(); ++section)
            changes += sections[section].insertChanges(sectionUpdates[section]);

        return changes;
    }

    Sections3d snapshotsToSections3d(const BlockStatesSnapshots& snapshots) {
        auto sections = std::vector<DeltaBlockStates>{};
        sections.reserve(snapshots.size());

        for (const auto& snapshot : snapshots)
            sections.emplace_back(snapshot);

        return sections;
    }
}