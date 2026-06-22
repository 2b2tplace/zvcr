#pragma once

#include <vector>
#include <ctime>
#include <result.hpp>

namespace zvcr {

    enum class SegmentStateType {
        UNKNOWN = 0,
        NEW = 1,
        OLD = 2
    };

    struct SegmentState {
        SegmentStateType type;
        time_t timestamp;

        auto operator==(const SegmentState &other) const -> bool ;
    };

    using SegmentStates = std::vector<SegmentState>;

    class SegmentInfo {
    public:
        SegmentStates segmentStates{};

        explicit SegmentInfo(const SegmentState &initialState);

        explicit SegmentInfo(const SegmentStates &segmentStates);

        SegmentInfo() = default;

        [[nodiscard]]
        auto latestSnapshot() const -> result::OptionCRef<SegmentState>;

        [[nodiscard]]
        auto delta(size_t deltaIndex) const -> result::OptionCRef<SegmentState>;

        [[nodiscard]]
        auto snapshotBefore(time_t timestamp) const -> result::Option<SegmentState>;

        [[nodiscard]]
        auto snapshotFrom(time_t timestamp) const -> result::Option<SegmentState>;

        [[nodiscard]]
        auto insertSnapshot(const SegmentState &newState) -> bool;
    };

}