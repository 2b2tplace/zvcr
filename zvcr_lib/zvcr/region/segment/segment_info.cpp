#include <zvcr/region/segment/segment_info.hpp>

namespace zvcr {

    SegmentInfo::SegmentInfo(const SegmentState& initialState) {
        this->segmentStates.push_back(initialState);
    }

    SegmentInfo::SegmentInfo(const SegmentStates& chunkStates) {
        this->segmentStates = chunkStates;
    }

    result::OptionCRef<SegmentState> SegmentInfo::latestState() const {
        return segmentStates.empty() ? result::None : result::Option{segmentStates[0]};
    }

    result::Option<SegmentState> SegmentInfo::stateFrom(const time_t timestamp) const {
        auto[latestStateType, _] = REQUIRE(this->latestState()).get();

        for (const auto& [type, deltaTimestamp] : segmentStates) {
            latestStateType = type;
            if (timestamp >= deltaTimestamp) break;
        }
        return SegmentState{latestStateType, timestamp};
    }

    bool SegmentInfo::updateState(const SegmentState& newState) {
        if (const auto latest = this->latestState();
            latest.has_value() && (newState.timestamp <= latest->get().timestamp || latest->get().type == newState.type)) return false;

        segmentStates.insert(segmentStates.begin(), newState);
        return true;
    }

}