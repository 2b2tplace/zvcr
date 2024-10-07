#pragma once

#include <libzr/modules/zr_common.hpp>
#include <libzr/modules/common/zr_region.hpp>
#include <libzr/modules/common/zr_delta.hpp>
#include <libzr/modules/common/zr_sector.hpp>

typedef std::vector<ZrDeltaBlockStates> Sections;

class ZvrChunk : public ZrSector {
public:
    explicit ZvrChunk(const Sections& sections, const ZrChunkState& initialState, const ZrTileEntityCounts& initialTileEntityCounts);
    explicit ZvrChunk(const Sections& sections, const ChunkStates& chunkStates, const TileEntities& tileEntities);

    ZrDeltaBlockStates getSection(uint8_t y) const;
    BlockStatesSnapshots snapshotFrom(time_t timestamp) const;
    BlockStatesSnapshots latestSnapshot() const;
    size_t updateSections(const BlockStatesSnapshots& sectionUpdates);

    Sections sections;
};

typedef ZrRegion<ZvrChunk> ZvrRegion;
typedef ZvrRegion::Segments Chunks;

Sections snapshotsToSections(const BlockStatesSnapshots& snapshots);