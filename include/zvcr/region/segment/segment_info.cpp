#include <zvcr/region/segment/segment_info.hpp>

namespace zvcr::region::segment::segment_info {

    SegmentInfo::SegmentInfo(const SegmentState& initialState, const TileEntityCountInfo& initialTileEntityCounts) {
        this->segmentStates.push_back(initialState);
        this->tileEntityCounts.push_back(initialTileEntityCounts);
    }

    SegmentInfo::SegmentInfo(const SegmentStates& chunkStates, const TileEntityCounts &tileEntities) {
        this->segmentStates = chunkStates;
        this->tileEntityCounts = tileEntities;
    }

    SegmentState SegmentInfo::latestState() const {
        return segmentStates[0];
    }

    TileEntityCountInfo SegmentInfo::latestTileEntityCounts() const {
        return tileEntityCounts[0];
    }

    SegmentState SegmentInfo::stateFrom(const time_t timestamp) const {
        auto latestStateType = this->latestState().type;
        for (const auto& [type, deltaTimestamp] : segmentStates) {
            latestStateType = type;
            if (timestamp >= deltaTimestamp) break;
        }
        return SegmentState {latestStateType, timestamp};
    }

    TileEntityCountInfo SegmentInfo::tileEntityCountsFrom(const time_t timestamp) const {
        auto latestCounts = this->latestTileEntityCounts().counts;
        for (const auto&[counts, deltaTimestamp] : tileEntityCounts) {
            latestCounts = counts;
            if (timestamp >= deltaTimestamp) break;
        }
        return TileEntityCountInfo {latestCounts, timestamp};
    }

    bool SegmentInfo::updateState(const SegmentState& newState) {
        if (auto [type, timestamp] = this->latestState();
            newState.timestamp <= timestamp || type == newState.type)
            return false;

        segmentStates.insert(segmentStates.begin(), newState);
        return true;
    }

    bool SegmentInfo::updateTileEntityCounts(const TileEntityCountInfo& newTileEntityCounts) {
        if (const auto [counts, timestamp] = this->latestTileEntityCounts();
            newTileEntityCounts.timestamp <= timestamp
            || counts == newTileEntityCounts.counts) return false;

        tileEntityCounts.insert(tileEntityCounts.begin(), newTileEntityCounts);
        return true;
    }

}