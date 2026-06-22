#include <zvcr/region/segment_info.hpp>
#include <zvcr/time_utils.hpp>

namespace zvcr {
    auto SegmentState::operator==(const SegmentState &other) const -> bool {
        return type == other.type && timestamp == other.timestamp;
    }

    SegmentInfo::SegmentInfo(const SegmentState &initialState) {
        this->segmentStates.push_back(initialState);
    }

    SegmentInfo::SegmentInfo(const SegmentStates &segmentStates) {
        this->segmentStates = segmentStates;
    }

    auto SegmentInfo::latestSnapshot() const -> result::OptionCRef<SegmentState> {
        return delta(0);
    }

    auto SegmentInfo::delta(const size_t deltaIndex) const -> result::OptionCRef<SegmentState> {
        return deltaIndex >= segmentStates.size() ? result::None : result::OptionCRef<SegmentState>{segmentStates[deltaIndex]};
    }

    auto SegmentInfo::snapshotBefore(const time_t timestamp) const -> result::Option<SegmentState> {
        auto[latestStateType, _] = REQUIRE(this->latestSnapshot()).get();

        for (const auto &[type, deltaTimestamp] : segmentStates) {
            latestStateType = type;
            if (timestamp >= deltaTimestamp) break;
        }
        return SegmentState{latestStateType, timestamp};
    }

    auto SegmentInfo::snapshotFrom(const time_t timestamp) const -> result::Option<SegmentState> {
        return snapshotBefore(findNearestTimestamp(segmentStates, timestamp));
    }

    auto SegmentInfo::insertSnapshot(const SegmentState &newState) -> bool {
        if (const auto latest = this->latestSnapshot();
            latest.has_value() && (newState.timestamp <= latest->get().timestamp || latest->get().type == newState.type)) return false;

        segmentStates.insert(segmentStates.begin(), newState);
        return true;
    }

}