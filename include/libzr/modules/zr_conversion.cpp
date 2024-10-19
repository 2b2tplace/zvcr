#include <libzr/modules/zr_conversion.hpp>

void TileViewDeltas::emplaceMissingView(const uint8_t layerType, const time_t timestamp) {
    auto& topDownViews = viewDeltas[layerType];

    if (!topDownViews.contains(timestamp))
        topDownViews.emplace(timestamp, TILE_SIZE);
}

ZrBlockStatesView& TileViewDeltas::deltaView(const ZprLayerType layerType, const time_t timestamp) {
    return deltaView(static_cast<uint8_t>(layerType), timestamp);
}

ZrBlockStatesView& TileViewDeltas::deltaView(const uint8_t layerType, const time_t timestamp) {
    emplaceMissingView(layerType, timestamp);
    return viewDeltas.at(layerType).at(timestamp);
}

ZrBlockStatesView& TileViewDeltas::topDown(const time_t timestamp) {
    return deltaView(ZprLayerType::TOP_DOWN, timestamp);
}

ZrBlockStatesView& TileViewDeltas::roofless(const time_t timestamp) {
    return deltaView(ZprLayerType::TOP_DOWN_ROOFLESS, timestamp);
}

ZrBlockStatesView& TileViewDeltas::heightmap(const time_t timestamp) {
    return deltaView(ZprLayerType::HEIGHTMAP, timestamp);
}

ZrBlockStatesView& TileViewDeltas::heightmapRoofless(const time_t timestamp) {
    return deltaView(ZprLayerType::HEIGHTMAP_ROOFLESS, timestamp);
}

ZprLayers TileViewDeltas::createLayers() const {
    ZprLayers layers;
    for (const auto&[layerType, deltas] : viewDeltas) {
        std::vector<ZrBlockStatesSnapshot> reverseDeltas;
        reverseDeltas.reserve(deltas.size());
        ZrDeltaBlockStates deltaBlockStates(reverseDeltas, TILE_SIZE);

        std::vector<time_t> timestamps;
        for (const auto &timestamp: deltas | std::views::keys)
            timestamps.push_back(timestamp);

        std::ranges::sort(timestamps, std::greater());

        for (const auto timestamp : timestamps) {
            const auto& view = deltas.at(timestamp);
            const auto packed = ZrBlockStates::pack(view.unpacked);
            const auto snapshot = ZrBlockStatesSnapshot{packed, timestamp};
            deltaBlockStates.insertChanges(snapshot);
        }
        layers[layerType] = ZprLayer{deltaBlockStates, layerType};
    }
    return layers;
}

ZprFile convertZvrFileToZprFile(const ZvrFile& zvrFile) {
    const auto [version, dimensionType, region] = zvrFile;
    const auto properties = DimensionTypePropertyRegistry.at(dimensionType);
    return ZprFile{ZPR_LATEST, dimensionType, convertZvrRegionToZprRegion(region, properties)};
}

ZprRegion convertZvrRegionToZprRegion(const ZvrRegion& zvrRegion, const ZvrDimensionProperties& properties) {
    ZprRegion zprRegion;
    for (size_t i = 0; i < SEGMENTS_PER_REGION; ++i)
        zprRegion.segments.push_back(convertZvrOptChunkToZprOptSegment(zvrRegion.segments[i], properties));

    return zprRegion;
}

std::optional<ZprSegment> convertZvrOptChunkToZprOptSegment(const std::optional<ZvrChunk>& zvrChunkOpt, const ZvrDimensionProperties& properties) {
    if (!zvrChunkOpt) return std::nullopt;

    return convertZvrChunkToZprSegment(zvrChunkOpt.value(), properties);
}

ZprSegment convertZvrChunkToZprSegment(const ZvrChunk& zvrChunk, const ZvrDimensionProperties& properties) {
    const auto sectionCount = properties.height / 16;
    TileViewDeltas tileViewDeltas;

    std::vector<time_t> timestamps;
    for (const auto& section : zvrChunk.sections) {
        for (const auto&[data, timestamp] : section.reverseDeltas) {
            if (std::ranges::find(timestamps, timestamp) == timestamps.end())
                timestamps.push_back(timestamp);
        }
    }
    std::ranges::sort(timestamps, std::greater());
    std::unordered_map<time_t, std::unordered_map<int8_t, UnpackedBlockStates>> cachedSnapshots;

    for (const auto timestamp : timestamps) {
        std::vector<ZrBlockStatesView> chunkAccumulator;
        for (auto sy = 0; sy < sectionCount; ++sy) {
            const auto& section = zvrChunk.sections[sy];

            UnpackedBlockStates snapshotBuilder;
            if (cachedSnapshots.contains(timestamp) && cachedSnapshots[timestamp].contains(sy))
                snapshotBuilder = cachedSnapshots[timestamp].at(sy);
            else {
                snapshotBuilder = section.latestSnapshot().data.unpack();
                for (const auto& [sectionData, deltaTimestamp] : section.reverseDeltas) {
                    const auto unpacked = sectionData.unpack();
                    for (size_t j = 0; j < section.snapshotLength; ++j) {
                        if (const auto state = unpacked[j]; state != STATE_UNCHANGED)
                            snapshotBuilder[j] = state;
                    }
                    cachedSnapshots[deltaTimestamp].emplace(sy, snapshotBuilder);
                }
            }
            chunkAccumulator.emplace_back(snapshotBuilder);
        }
        for (auto sy = sectionCount - 1; sy >= 0; --sy)
            renderZprSegmentForSectionSnapshot(timestamp, sy, chunkAccumulator[sy], tileViewDeltas);
    }
    return ZprSegment(tileViewDeltas.createLayers(), zvrChunk.chunkStates, zvrChunk.tileEntities);
}

void renderZprSegment(const uint8_t cx, const uint8_t cz, const uint8_t sy,
                      const ZrBlockStatesView& sectionView,
                      ZrBlockStatesView& topDownTileView,
                      ZrBlockStatesView& heightmapTileView,
                      const bool ignoreRoof) {
    const auto current = topDownTileView.get(cx, cz);
    if (current != 0) return;

    for (int8_t cy = CHUNK_SIDELENGTH - 1; cy >= 0; --cy) {
        const auto state = sectionView.getBlockState(cx, cy, cz);
        if (state == 0) continue;

        if (ignoreRoof && (state == 79 /* bedrock */
                || state == 2354 /* obsidian */
                || state == 19449 /* crying obsidian */
                || (state >= 5772 && state <= 5779) /* snow layers 1-8 */)) continue;

        topDownTileView.set(cx, cz, state);

        if (const auto height = sy * 16 + cy; height > heightmapTileView.get(cx, cz))
            heightmapTileView.set(cx, cz, height);

        break;
    }
}

void renderZprSegmentForSectionSnapshot(const time_t timestamp, const uint8_t sy,
    const ZrBlockStatesView& sectionView, TileViewDeltas& tileViewDeltas) {

    auto& topDownTileView = tileViewDeltas.topDown(timestamp);
    auto& rooflessTileView = tileViewDeltas.roofless(timestamp);
    auto& heightmapTileView = tileViewDeltas.heightmap(timestamp);
    auto& heightmapRooflessTileView = tileViewDeltas.heightmapRoofless(timestamp);

    for (uint8_t cx = 0; cx < CHUNK_SIDELENGTH; ++cx) {
        for (uint8_t cz = 0; cz < CHUNK_SIDELENGTH; ++cz) {
            renderZprSegment(cx, cz, sy, sectionView, topDownTileView, heightmapTileView, false);
            renderZprSegment(cx, cz, sy, sectionView, rooflessTileView, heightmapRooflessTileView, true);
        }
    }
}