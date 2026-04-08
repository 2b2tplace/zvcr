#include <zvcr/region/segment/segment_info.hpp>

namespace zvcr {

    SegmentInfo::SegmentInfo(const SegmentState& initialState) {
        this->segmentStates.push_back(initialState);
    }

    SegmentInfo::SegmentInfo(const SegmentStates& chunkStates) {
        this->segmentStates = chunkStates;
    }

    auto SegmentInfo::latestState() const -> result::OptionCRef<SegmentState> {
        return delta(0);
    }

    auto SegmentInfo::delta(const size_t deltaIndex) const -> result::OptionCRef<SegmentState> {
        return deltaIndex >= segmentStates.size() ? result::None : result::OptionCRef<SegmentState>{segmentStates[deltaIndex]};
    }

    auto SegmentInfo::stateFrom(const time_t timestamp) const -> result::Option<SegmentState> {
        auto[latestStateType, _] = REQUIRE(this->latestState()).get();

        for (const auto& [type, deltaTimestamp] : segmentStates) {
            latestStateType = type;
            if (timestamp >= deltaTimestamp) break;
        }
        return SegmentState{latestStateType, timestamp};
    }

    auto SegmentInfo::updateState(const SegmentState& newState) -> bool {
        if (const auto latest = this->latestState();
            latest.has_value() && (newState.timestamp <= latest->get().timestamp || latest->get().type == newState.type)) return false;

        segmentStates.insert(segmentStates.begin(), newState);
        return true;
    }

}