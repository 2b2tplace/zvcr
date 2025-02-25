#include <zvcr/common/reverse_delta.hpp>

namespace zvcr::common::reverse_delta {

    DeltaBlockStates::DeltaBlockStates(const size_t snapshotLength) {
        this->snapshotLength = snapshotLength;
        this->reverseDeltas = {};
    }

    DeltaBlockStates::DeltaBlockStates(const BlockStatesSnapshot& initialState) {
        this->snapshotLength = initialState.data.snapshotLength;
        this->reverseDeltas.push_back(initialState);
    }

    DeltaBlockStates::DeltaBlockStates(const std::vector<BlockStatesSnapshot>& reverseDeltas, const size_t snapshotLength) {
        this->snapshotLength = snapshotLength;
        this->reverseDeltas = reverseDeltas;
    }

    std::optional<BlockStatesSnapshot> DeltaBlockStates::latestSnapshot() const {
        return delta(0);
    }

    std::optional<BlockStatesSnapshot> DeltaBlockStates::delta(const size_t deltaIndex) const {
        return reverseDeltas.empty() ? std::nullopt : std::optional(reverseDeltas[deltaIndex]);
    }

    std::optional<BlockStatesSnapshot> DeltaBlockStates::snapshotFrom(const time_t timestamp) const {
        const auto latest = this->latestSnapshot();
        if (!latest.has_value()) return std::nullopt;

        auto latestSnapshot = latest->data.unpack();
        bool first = true;
        for (const auto& [sectionData, deltaTimestamp] : reverseDeltas) {
            if (first) {
                first = false;
                continue;
            }
            if (timestamp > deltaTimestamp) break;

            const auto unpacked = sectionData.unpack();
            for (size_t j = 0; j < snapshotLength; ++j) {
                if (const auto state = unpacked[j]; state != STATE_UNCHANGED)
                    latestSnapshot[j] = state;
            }
        }
        return BlockStatesSnapshot {BlockStates::pack(latestSnapshot), timestamp};
    }

    DeltaInsertionResult DeltaBlockStates::insertSnapshot(const BlockStatesSnapshot& newSnapshot) {
        const auto latest = latestSnapshot();
        if (!latest.has_value()) {
            reverseDeltas.push_back(newSnapshot);
            return newSnapshot.data.snapshotLength;
        }
        if (newSnapshot.data.snapshotLength != this->snapshotLength)
            return result::Error(DeltaInsertionStatus::INVALID_SNAPSHOT_LENGTH);

        const auto [sectionData, timestamp] = *latest;

        if (newSnapshot.timestamp <= timestamp)
            return result::Error(DeltaInsertionStatus::SNAPSHOT_OLDER_THAN_LATEST);

        paletted_storage::UnpackedBlockStates deltaSnapshotBuilder(snapshotLength);

        const auto previousUnpacked = sectionData.unpack();
        const auto newUnpacked = newSnapshot.data.unpack();

        size_t changes = 0;
        for (size_t i = 0; i < snapshotLength; ++i) {
            const auto previous = previousUnpacked[i];
            const bool changed = newUnpacked[i] != previous;
            deltaSnapshotBuilder[i] = changed ? previous : STATE_UNCHANGED;

            if (changed) changes++;
        }
        if (changes == 0)
            return result::Error(DeltaInsertionStatus::NO_CHANGES_MADE);

        const auto deltaSnapshot = BlockStatesSnapshot {
            BlockStates::pack(deltaSnapshotBuilder),
            timestamp
        };
        reverseDeltas.erase(reverseDeltas.begin());
        reverseDeltas.insert(reverseDeltas.begin(), deltaSnapshot);
        reverseDeltas.insert(reverseDeltas.begin(), newSnapshot);
        return changes;
    }

}
