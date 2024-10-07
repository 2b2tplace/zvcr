#pragma once

#include <libzr/modules/common/zr_chunk_state.hpp>
#include <libzr/modules/common/zr_tile_entities.hpp>

class ZrSector {
public:
    explicit ZrSector(const ZrChunkState& initialState, const ZrTileEntityCounts& initialTileEntityCounts);
    explicit ZrSector(const ChunkStates& chunkStates, const TileEntities& tileEntities);

    ZrChunkState latestState() const;
    ZrTileEntityCounts latestTileEntityCounts() const;
    ZrChunkState stateFrom(time_t timestamp) const;
    ZrTileEntityCounts tileEntityCountsFrom(time_t timestamp) const;
    bool updateChunkState(const ZrChunkState& newState);
    bool updateTileEntityCounts(const ZrTileEntityCounts& newTileEntityCounts);

    ChunkStates chunkStates;
    TileEntities tileEntities;
};