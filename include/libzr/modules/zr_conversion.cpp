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

ZrBlockStatesView& TileViewDeltas::drainedTopDown(const time_t timestamp) {
    return deltaView(ZprLayerType::DRAINED_TOP_DOWN, timestamp);
}

ZrBlockStatesView& TileViewDeltas::drainedTopDownHeightmap(const time_t timestamp) {
    return deltaView(ZprLayerType::DRAINED_TOP_DOWN_HEIGHTMAP, timestamp);
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
    const auto sectionCount = static_cast<int8_t>(properties.height / 16);
    TileViewDeltas tileViewDeltas;
    std::vector<time_t> timestamps;
    for (const auto& section : zvrChunk.sections) {
        for (const auto&[data, timestamp] : section.reverseDeltas) {
            if (std::ranges::find(timestamps, timestamp) == timestamps.end())
                timestamps.push_back(timestamp);
        }
    }
    std::unordered_map<time_t, std::unordered_map<int8_t, ZrBlockStatesView>> cachedSnapshots;

    for (const auto ts : timestamps) {
        for (auto sy = static_cast<int8_t>(sectionCount - 1); sy >= 0; --sy) {
            const auto& section = zvrChunk.sections[sy];
            const auto&[data, timestamp] = section.latestSnapshot();

            auto snapshotBuilder = data.unpack();
            cachedSnapshots[timestamp].emplace(sy, snapshotBuilder);

            bool first = true;
            for (const auto& [sectionData, deltaTimestamp] : section.reverseDeltas) {
                if (first) {
                    first = false;
                    continue;
                }
                const auto unpacked = sectionData.unpack();
                for (size_t j = 0; j < section.snapshotLength; ++j) {
                    if (const auto state = unpacked[j]; state != STATE_UNCHANGED)
                        snapshotBuilder[j] = state;
                }
                cachedSnapshots[deltaTimestamp].emplace(sy, snapshotBuilder);
            }
            if (!cachedSnapshots[ts].contains(sy))
                cachedSnapshots[ts].emplace(sy, section.snapshotFrom(ts).data.unpack());

            if (renderZprSegmentForSectionSnapshot(ts, sy, cachedSnapshots[ts].at(sy), tileViewDeltas)) break;
        }
    }
    return ZprSegment(tileViewDeltas.createLayers(), zvrChunk.chunkStates, zvrChunk.tileEntities);
}

bool renderZprSegment(const uint8_t cx, const uint8_t cz, const uint8_t sy,
                      const ZrBlockStatesView& sectionView,
                      ZrBlockStatesView& topDownTileView,
                      ZrBlockStatesView& heightmapTileView,
                      const bool ignoreRoof,
                      const bool ignoreWater) {
    const auto current = topDownTileView.get(cx, cz);
    if (current != 0) return true;

    for (int8_t cy = CHUNK_SIDELENGTH - 1; cy >= 0; --cy) {
        const auto state = sectionView.getBlockState(cx, cy, cz);
        if (state == 0) continue;

        if (ignoreWater && state >= 80 && state <= 111) continue;
        if (ignoreRoof && (state == 79 /* bedrock */
                || state == 2354 /* obsidian */
                || state == 19449 /* crying obsidian */
                || (state >= 5772 && state <= 5779) /* snow layers 1-8 */)) continue;

        topDownTileView.set(cx, cz, state);

        if (const auto height = sy * 16 + cy; height > heightmapTileView.get(cx, cz)) {
            heightmapTileView.set(cx, cz, height);
            return true;
        }
    }
    return false;
}

bool renderZprSegmentForSectionSnapshot(const time_t timestamp, const uint8_t sy,
    const ZrBlockStatesView& sectionView, TileViewDeltas& tileViewDeltas) {

    auto& topDownTileView = tileViewDeltas.topDown(timestamp);
    auto& rooflessTileView = tileViewDeltas.roofless(timestamp);
    auto& heightmapTileView = tileViewDeltas.heightmap(timestamp);
    auto& heightmapRooflessTileView = tileViewDeltas.heightmapRoofless(timestamp);
    auto& drainedTopDownTileView = tileViewDeltas.drainedTopDown(timestamp);
    auto& drainedTopDownHeightmapTileView = tileViewDeltas.drainedTopDown(timestamp);

    bool finished = true;

    for (uint8_t cx = 0; cx < CHUNK_SIDELENGTH; ++cx) {
        for (uint8_t cz = 0; cz < CHUNK_SIDELENGTH; ++cz) {
            finished &= renderZprSegment(cx, cz, sy, sectionView, topDownTileView, heightmapTileView, false, false);
            finished &= renderZprSegment(cx, cz, sy, sectionView, drainedTopDownTileView, drainedTopDownHeightmapTileView, false, true);
            finished &= renderZprSegment(cx, cz, sy, sectionView, rooflessTileView, heightmapRooflessTileView, true, false);
        }
    }
    return finished;
}