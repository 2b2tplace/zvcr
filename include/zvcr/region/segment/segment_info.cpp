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

    std::optional<SegmentState> SegmentInfo::latestState() const {
        return segmentStates.empty() ? std::nullopt : std::optional(segmentStates[0]);
    }

    std::optional<TileEntityCountInfo> SegmentInfo::latestTileEntityCounts() const {
        return tileEntityCounts.empty() ? std::nullopt : std::optional(tileEntityCounts[0]);
    }

    std::optional<SegmentState> SegmentInfo::stateFrom(const time_t timestamp) const {
        const auto latest = this->latestState();
        if (!latest.has_value()) return std::nullopt;

        auto latestStateType = latest->type;
        for (const auto& [type, deltaTimestamp] : segmentStates) {
            latestStateType = type;
            if (timestamp >= deltaTimestamp) break;
        }
        return SegmentState {latestStateType, timestamp};
    }

    std::optional<TileEntityCountInfo> SegmentInfo::tileEntityCountsFrom(const time_t timestamp) const {
        const auto latest = this->latestTileEntityCounts();
        if (!latest.has_value()) return std::nullopt;

        auto latestCounts = latest->counts;
        for (const auto&[counts, deltaTimestamp] : tileEntityCounts) {
            latestCounts = counts;
            if (timestamp >= deltaTimestamp) break;
        }
        return TileEntityCountInfo {latestCounts, timestamp};
    }

    bool SegmentInfo::updateState(const SegmentState& newState) {
        if (const auto latest = this->latestState();
            latest.has_value() && (newState.timestamp <= latest->timestamp || latest->type == newState.type)) return false;

        segmentStates.insert(segmentStates.begin(), newState);
        return true;
    }

    bool SegmentInfo::updateTileEntityCounts(const TileEntityCountInfo& newTileEntityCounts) {
        if (const auto latest = this->latestTileEntityCounts();
            latest.has_value() && (newTileEntityCounts.timestamp <= latest->timestamp
            || latest->counts == newTileEntityCounts.counts)) return false;

        tileEntityCounts.insert(tileEntityCounts.begin(), newTileEntityCounts);
        return true;
    }

}