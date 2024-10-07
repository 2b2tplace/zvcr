#include <libzr/modules/common/zr_sector.hpp>

ZrSector::ZrSector(const ZrChunkState& initialState, const ZrTileEntityCounts& initialTileEntityCounts) {
    this->chunkStates.push_back(initialState);
    this->tileEntities.push_back(initialTileEntityCounts);
}

ZrSector::ZrSector(const ChunkStates& chunkStates, const TileEntities &tileEntities) {
    this->chunkStates = chunkStates;
    this->tileEntities = tileEntities;
}

ZrChunkState ZrSector::latestState() const {
    return chunkStates[0];
}

ZrTileEntityCounts ZrSector::latestTileEntityCounts() const {
    return tileEntities[0];
}

ZrChunkState ZrSector::stateFrom(const time_t timestamp) const {
    auto latestStateType = this->latestState().type;
    for (const auto& [type, deltaTimestamp] : chunkStates) {
        latestStateType = type;
        if (timestamp >= deltaTimestamp) break;
    }
    return ZrChunkState {latestStateType, timestamp};
}

ZrTileEntityCounts ZrSector::tileEntityCountsFrom(const time_t timestamp) const {
    auto latestCounts = this->latestTileEntityCounts().counts;
    for (const auto&[counts, deltaTimestamp] : tileEntities) {
        latestCounts = counts;
        if (timestamp >= deltaTimestamp) break;
    }
    return ZrTileEntityCounts {latestCounts, timestamp};
}

bool ZrSector::updateChunkState(const ZrChunkState& newState) {
    if (auto [type, timestamp] = this->latestState();
        newState.timestamp <= timestamp || type == newState.type)
        return false;

    chunkStates.insert(chunkStates.begin(), newState);
    return true;
}

bool ZrSector::updateTileEntityCounts(const ZrTileEntityCounts& newTileEntityCounts) {
    if (const auto [counts, timestamp] = this->latestTileEntityCounts();
        newTileEntityCounts.timestamp <= timestamp
        || counts == newTileEntityCounts.counts) return false;

    tileEntities.insert(tileEntities.begin(), newTileEntityCounts);
    return true;
}