#include <libzr/modules/zvr/zvr_chunk.hpp>

ZvrChunk::ZvrChunk(const Sections& sections, const ZrChunkState& initialState, const ZrTileEntityCounts& initialTileEntityCounts): ZrSector(initialState, initialTileEntityCounts) {
    this->sections = sections;
}

ZvrChunk::ZvrChunk(const Sections& sections, const ChunkStates& chunkStates, const TileEntities& tileEntities): ZrSector(chunkStates, tileEntities) {
    this->sections = sections;
}

ZrDeltaBlockStates ZvrChunk::getSection(const uint8_t y) const {
    return sections[y];
}

BlockStatesSnapshots ZvrChunk::snapshotFrom(const time_t timestamp) const {
    BlockStatesSnapshots snapshots;
    snapshots.reserve(this->sections.size());

    for (const auto & section : this->sections)
        snapshots.push_back(section.snapshotFrom(timestamp));

    return snapshots;
}

BlockStatesSnapshots ZvrChunk::latestSnapshot() const {
    BlockStatesSnapshots snapshots;
    snapshots.reserve(this->sections.size());

    for (const auto & section : this->sections)
        snapshots.push_back(section.latestSnapshot());

    return snapshots;
}

size_t ZvrChunk::updateSections(const BlockStatesSnapshots &sectionUpdates) {
    size_t changes = 0;
    for (size_t section = 0; section < sectionUpdates.size(); ++section) {
        changes += sections[section].insertChanges(sectionUpdates[section]);
    }
    return changes;
}

Sections snapshotsToSections(const BlockStatesSnapshots& snapshots) {
    auto sections = std::vector<ZrDeltaBlockStates>();
    sections.reserve(snapshots.size());

    for (const auto & snapshot : snapshots)
        sections.emplace_back(snapshot);

    return sections;
}