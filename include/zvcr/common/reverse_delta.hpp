#pragma once

#include <zvcr/common/paletted_storage.hpp>
#include <ctime>
#include <vector>

namespace zvcr::common::reverse_delta {

    using paletted_storage::BlockStates;

    static constexpr uint16_t STATE_UNCHANGED = 0xFFFF;

    struct BlockStatesSnapshot {
        BlockStates data;
        time_t timestamp{};
    };

    using BlockStatesSnapshots = std::vector<BlockStatesSnapshot>;

    class DeltaBlockStates {
    public:
        size_t snapshotLength;
        std::vector<BlockStatesSnapshot> reverseDeltas;

        explicit DeltaBlockStates(const BlockStatesSnapshot& initialState);

        explicit DeltaBlockStates(const std::vector<BlockStatesSnapshot>& reverseDeltas, size_t snapshotLength);

        [[nodiscard]]
        BlockStatesSnapshot latestSnapshot() const;

        [[nodiscard]]
        BlockStatesSnapshot delta(size_t deltaIndex) const;

        [[nodiscard]]
        BlockStatesSnapshot snapshotFrom(time_t timestamp) const;

        size_t insertChanges(const BlockStatesSnapshot& newSnapshot);
    };

}