#include <libzr/modules/common/zr_delta.hpp>

ZrDeltaBlockStates::ZrDeltaBlockStates(const ZrBlockStatesSnapshot& initialState) {
    this->snapshotLength = initialState.data.snapshotLength;
    this->reverseDeltas.push_back(initialState);
}

ZrDeltaBlockStates::ZrDeltaBlockStates(const std::vector<ZrBlockStatesSnapshot>& reverseDeltas, const size_t snapshotLength) {
    this->snapshotLength = snapshotLength;
    this->reverseDeltas = reverseDeltas;
}

ZrBlockStatesSnapshot ZrDeltaBlockStates::latestSnapshot() const {
    return delta(0);
}

ZrBlockStatesSnapshot ZrDeltaBlockStates::delta(const size_t deltaIndex) const {
    return reverseDeltas[deltaIndex];
}

ZrBlockStatesSnapshot ZrDeltaBlockStates::snapshotFrom(const time_t timestamp) const {
    auto latestSnapshot = this->latestSnapshot().data.unpack();
    for (const auto& [sectionData, deltaTimestamp] : reverseDeltas) {
        const auto unpacked = sectionData.unpack();
        for (size_t j = 0; j < snapshotLength; ++j) {
            if (const auto state = unpacked[j]; state != STATE_UNCHANGED)
                latestSnapshot[j] = state;
        }
        if (timestamp >= deltaTimestamp) break;
    }
    return ZrBlockStatesSnapshot {ZrBlockStates::pack(latestSnapshot), timestamp};
}

size_t ZrDeltaBlockStates::insertChanges(const ZrBlockStatesSnapshot& newSnapshot) {
    const auto [sectionData, timestamp] = latestSnapshot();

    if (!reverseDeltas.empty() && newSnapshot.timestamp <= timestamp) return 0;

    UnpackedBlockStates deltaSnapshotBuilder(snapshotLength);

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

    const auto deltaSnapshot = ZrBlockStatesSnapshot {
        ZrBlockStates::pack(deltaSnapshotBuilder),
        timestamp
    };
    reverseDeltas.erase(reverseDeltas.begin());
    reverseDeltas.insert(reverseDeltas.begin(), deltaSnapshot);
    reverseDeltas.insert(reverseDeltas.begin(), newSnapshot);
    return changes;
}