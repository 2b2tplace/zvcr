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
    };

    using SegmentStates = std::vector<SegmentState>;

    class SegmentInfo {
    public:
        SegmentStates segmentStates{};

        explicit SegmentInfo(const SegmentState& initialState);

        explicit SegmentInfo(const SegmentStates& chunkStates);

        SegmentInfo() = default;

        [[nodiscard]]
        result::OptionCRef<SegmentState> latestState() const;

        [[nodiscard]]
        result::Option<SegmentState> stateFrom(time_t timestamp) const;

        [[nodiscard]]
        bool updateState(const SegmentState& newState);
    };

}