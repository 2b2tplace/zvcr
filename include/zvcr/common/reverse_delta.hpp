#pragma once

#include <ctime>
#include <vector>
#include <optional>
#include <zvcr/common/result.hpp>
#include <zvcr/common/paletted_storage.hpp>

namespace zvcr::common::reverse_delta {

    using paletted_storage::BlockStates;

    static constexpr uint16_t STATE_UNCHANGED = 0xFFFF;

    struct BlockStatesSnapshot {
        BlockStates data;
        time_t timestamp{};
    };

    enum class DeltaInsertionStatus {
        INVALID_SNAPSHOT_LENGTH,
        SNAPSHOT_OLDER_THAN_LATEST,
        NO_CHANGES_MADE
    };

    using DeltaInsertionResult = result::Result<size_t, DeltaInsertionStatus>;

    using BlockStatesSnapshots = std::vector<BlockStatesSnapshot>;

    class DeltaBlockStates {
    public:
        size_t snapshotLength;
        std::vector<BlockStatesSnapshot> reverseDeltas;

        explicit DeltaBlockStates(size_t snapshotLength);

        explicit DeltaBlockStates(const BlockStatesSnapshot& initialState);

        explicit DeltaBlockStates(const std::vector<BlockStatesSnapshot>& reverseDeltas, size_t snapshotLength);

        [[nodiscard]]
        std::optional<BlockStatesSnapshot> latestSnapshot() const;

        [[nodiscard]]
        std::optional<BlockStatesSnapshot> delta(size_t deltaIndex) const;

        [[nodiscard]]
        std::optional<BlockStatesSnapshot> snapshotFrom(time_t timestamp) const;

        [[nodiscard]]
        DeltaInsertionResult insertSnapshot(const BlockStatesSnapshot& newSnapshot);
    };

}
