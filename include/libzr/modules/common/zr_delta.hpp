#pragma once

#include <libzr/modules/zr_common.hpp>
#include <libzr/modules/common/zr_paletted_storage.hpp>

#define STATE_UNCHANGED 0xFFFF

struct ZrBlockStatesSnapshot {
    ZrBlockStates data;
    time_t timestamp{};
};

class ZrDeltaBlockStates {
public:
    explicit ZrDeltaBlockStates(const ZrBlockStatesSnapshot& initialState);
    explicit ZrDeltaBlockStates(const std::vector<ZrBlockStatesSnapshot>& reverseDeltas, size_t snapshotLength);
    explicit ZrDeltaBlockStates();

    ZrBlockStatesSnapshot latestSnapshot() const;
    ZrBlockStatesSnapshot delta(size_t deltaIndex) const;
    ZrBlockStatesSnapshot snapshotFrom(time_t timestamp) const;
    size_t insertChanges(const ZrBlockStatesSnapshot& newSnapshot);

    size_t snapshotLength;
    std::vector<ZrBlockStatesSnapshot> reverseDeltas;
};

typedef std::vector<ZrBlockStatesSnapshot> BlockStatesSnapshots;