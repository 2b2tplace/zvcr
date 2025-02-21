#include <zvcr/common/reverse_delta.hpp>

namespace zvcr::common::reverse_delta {

    DeltaBlockStates::DeltaBlockStates(const BlockStatesSnapshot& initialState) {
        this->snapshotLength = initialState.data.snapshotLength;
        this->reverseDeltas.push_back(initialState);
    }

    DeltaBlockStates::DeltaBlockStates(const std::vector<BlockStatesSnapshot>& reverseDeltas, const size_t snapshotLength) {
        this->snapshotLength = snapshotLength;
        this->reverseDeltas = reverseDeltas;
    }

    BlockStatesSnapshot DeltaBlockStates::latestSnapshot() const {
        return delta(0);
    }

    BlockStatesSnapshot DeltaBlockStates::delta(const size_t deltaIndex) const {
        return reverseDeltas[deltaIndex];
    }

    BlockStatesSnapshot DeltaBlockStates::snapshotFrom(const time_t timestamp) const {
        auto latestSnapshot = this->latestSnapshot().data.unpack();
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

    size_t DeltaBlockStates::insertChanges(const BlockStatesSnapshot& newSnapshot) {
        if (reverseDeltas.empty()) {
            reverseDeltas.push_back(newSnapshot);
            return newSnapshot.data.snapshotLength;
        }
        const auto [sectionData, timestamp] = latestSnapshot();

        if (!reverseDeltas.empty() && newSnapshot.timestamp <= timestamp) return 0;

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
        if (changes == 0) return 0;

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