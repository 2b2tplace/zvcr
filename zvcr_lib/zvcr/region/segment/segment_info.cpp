#include <zvcr/region/segment/segment_info.hpp>

namespace zvcr {

    SegmentInfo::SegmentInfo(const SegmentState& initialState, const TileEntityCountInfo& initialTileEntityCounts) {
        this->segmentStates.push_back(initialState);
        this->tileEntityCounts.push_back(initialTileEntityCounts);
    }

    SegmentInfo::SegmentInfo(const SegmentStates& chunkStates, const TileEntityCounts& tileEntities) {
        this->segmentStates = chunkStates;
        this->tileEntityCounts = tileEntities;
    }

    result::OptionCRef<SegmentState> SegmentInfo::latestState() const {
        return segmentStates.empty() ? result::None : result::Option{segmentStates[0]};
    }

    result::OptionCRef<TileEntityCountInfo> SegmentInfo::latestTileEntityCounts() const {
        return tileEntityCounts.empty() ? result::None : result::Option{tileEntityCounts[0]};
    }

    result::Option<SegmentState> SegmentInfo::stateFrom(const time_t timestamp) const {
        auto[latestStateType, _] = REQUIRE(this->latestState()).get();

        for (const auto& [type, deltaTimestamp] : segmentStates) {
            latestStateType = type;
            if (timestamp >= deltaTimestamp) break;
        }
        return SegmentState{latestStateType, timestamp};
    }

    result::Option<TileEntityCountInfo> SegmentInfo::tileEntityCountsFrom(const time_t timestamp) const {
        auto [latestCounts, _] = REQUIRE(this->latestTileEntityCounts()).get();

        for (const auto&[counts, deltaTimestamp] : tileEntityCounts) {
            latestCounts = counts;
            if (timestamp >= deltaTimestamp) break;
        }
        return TileEntityCountInfo{latestCounts, timestamp};
    }

    bool SegmentInfo::updateState(const SegmentState& newState) {
        if (const auto latest = this->latestState();
            latest.has_value() && (newState.timestamp <= latest->get().timestamp || latest->get().type == newState.type)) return false;

        segmentStates.insert(segmentStates.begin(), newState);
        return true;
    }

    bool SegmentInfo::updateTileEntityCounts(const TileEntityCountInfo& newTileEntityCounts) {
        if (const auto latest = this->latestTileEntityCounts();
            latest.has_value() && (newTileEntityCounts.timestamp <= latest->get().timestamp
            || latest->get().counts == newTileEntityCounts.counts)) return false;

        tileEntityCounts.insert(tileEntityCounts.begin(), newTileEntityCounts);
        return true;
    }

}