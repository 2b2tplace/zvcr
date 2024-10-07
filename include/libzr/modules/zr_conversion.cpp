#include <libzr/modules/zr_conversion.hpp>

void TileViewDeltas::emplaceMissingView(const uint8_t layerType, const time_t timestamp) {
    auto& topDownViews = viewDeltas[layerType];
    if (topDownViews.contains(timestamp)) return;

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

ZprLayers TileViewDeltas::createLayers() const {
    ZprLayers layers;
    for (const auto&[layerType, deltas] : viewDeltas) {
        std::vector<ZrBlockStatesSnapshot> reverseDeltas;
        reverseDeltas.reserve(deltas.size());

        std::vector<time_t> timestamps;
        for (const auto &timestamp: deltas | std::views::keys)
            timestamps.push_back(timestamp);

        std::ranges::sort(timestamps, std::greater<time_t>());

        for (const auto timestamp : timestamps) {
            const auto& view = deltas.at(timestamp);
            const auto packed = ZrBlockStates::pack(view.unpacked);
            const auto snapshot = ZrBlockStatesSnapshot{packed, timestamp};
            reverseDeltas.push_back(snapshot);
        }
        layers[layerType] = ZprLayer{ZrDeltaBlockStates(reverseDeltas, TILE_SIZE), layerType};
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

    const auto& sections = zvrChunk.sections;
    for (int8_t sy = sectionCount - 1; sy >= 0; --sy) {
        for (const auto& section = sections[sy];
            const auto&[data, timestamp] : section.reverseDeltas) {
            const auto sectionView = ZrBlockStatesView(data.unpack());

            renderZprSegmentForSectionSnapshot(timestamp, sy, sectionView, tileViewDeltas);
        }
    }
    return ZprSegment(tileViewDeltas.createLayers(), zvrChunk.chunkStates, zvrChunk.tileEntities);
}

void renderZprSegmentForSectionSnapshot(const time_t timestamp, const uint8_t sy,
    const ZrBlockStatesView& sectionView, TileViewDeltas& tileViewDeltas) {

    auto& topDownTileView = tileViewDeltas.topDown(timestamp);
    auto& rooflessTileView = tileViewDeltas.roofless(timestamp);
    auto& heightmapTileView = tileViewDeltas.heightmap(timestamp);

    for (uint8_t cx = 0; cx < CHUNK_SIDELENGTH; ++cx) {
        for (uint8_t cz = 0; cz < CHUNK_SIDELENGTH; ++cz) {
            if (topDownTileView.get(cx, cz) != 0) continue;

            for (int8_t cy = CHUNK_SIDELENGTH - 1; cy >= 0; --cy) {
                const auto state = sectionView.getBlockState(cx, cy, cz);
                if (state == 0) continue;

                topDownTileView.set(cx, cz, state);

                if (const auto height = sy * 16 + cy; height > heightmapTileView.get(cx, cz))
                    heightmapTileView.set(cx, cz, height);

                if (state == 79 /* bedrock */
                    || state == 2354 /* obsidian */
                    || state == 19449 /* crying obsidian */
                    || (state >= 5772 && state <= 5779) /* snow layers 1-8 */) continue;

                rooflessTileView.set(cx, cz, state);
                break;
            }
        }
    }
}