#include <zvcr/region/segment/segment_info.hpp>

namespace zvcr::region::segment::segment_info {

    SegmentInfo::SegmentInfo(const SegmentState& initialState, const TileEntityCountInfo& initialTileEntityCounts) {
        this->segmentStates.push_back(initialState);
        this->tileEntityCounts.push_back(initialTileEntityCounts);
    }

    SegmentInfo::SegmentInfo(const SegmentStates& chunkStates, const TileEntityCounts& tileEntities) {
        this->segmentStates = chunkStates;
        this->tileEntityCounts = tileEntities;
    }

    OptionCRef<SegmentState> SegmentInfo::latestState() const {
        return segmentStates.empty() ? OptionCRef<SegmentState>{} : OptionCRef{segmentStates[0]};
    }

    OptionCRef<TileEntityCountInfo> SegmentInfo::latestTileEntityCounts() const {
        return tileEntityCounts.empty() ? OptionCRef<TileEntityCountInfo>{} : OptionCRef{tileEntityCounts[0]};
    }

    Option<SegmentState> SegmentInfo::stateFrom(const time_t timestamp) const {
        const auto& latest = Require(this->latestState());

        auto latestStateType = latest.type;
        for (const auto& [type, deltaTimestamp] : segmentStates) {
            latestStateType = type;
            if (timestamp >= deltaTimestamp) break;
        }
        return SegmentState{latestStateType, timestamp};
    }

    Option<TileEntityCountInfo> SegmentInfo::tileEntityCountsFrom(const time_t timestamp) const {
        const auto latest = Require(this->latestTileEntityCounts());

        auto latestCounts = latest.counts;
        for (const auto&[counts, deltaTimestamp] : tileEntityCounts) {
            latestCounts = counts;
            if (timestamp >= deltaTimestamp) break;
        }
        return TileEntityCountInfo{latestCounts, timestamp};
    }

    bool SegmentInfo::updateState(const SegmentState& newState) {
        if (const auto latest = this->latestState();
            latest.some() && (newState.timestamp <= latest->timestamp || latest->type == newState.type)) return false;

        segmentStates.insert(segmentStates.begin(), newState);
        return true;
    }

    bool SegmentInfo::updateTileEntityCounts(const TileEntityCountInfo& newTileEntityCounts) {
        if (const auto latest = this->latestTileEntityCounts();
            latest.some() && (newTileEntityCounts.timestamp <= latest->timestamp
            || latest->counts == newTileEntityCounts.counts)) return false;

        tileEntityCounts.insert(tileEntityCounts.begin(), newTileEntityCounts);
        return true;
    }

}