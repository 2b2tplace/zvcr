#include <ranges>
#include <algorithm>
#include <zvcr/serialize/conversion.hpp>
#include <zvcr/common/definitions.hpp>

namespace zvcr::serialize::conversion {

    using namespace common::definitions;
    
    void TileViewDeltas::emplaceMissingView(const uint8_t layerType, const time_t timestamp) {
        if (auto& topDownViews = viewDeltas[layerType]; !topDownViews.contains(timestamp))
            topDownViews.emplace(timestamp, UnpackedView<SegmentAtom>{SEGMENT_SIDELENGTH_BLOCKS, SECTION_2D_SIZE_BLOCKS});
    }

    UnpackedView<SegmentAtom>& TileViewDeltas::deltaView(const LayerType layerType, const time_t timestamp) {
        return deltaView(static_cast<uint8_t>(layerType), timestamp);
    }

    UnpackedView<SegmentAtom>& TileViewDeltas::deltaView(const uint8_t layerType, const time_t timestamp) {
        emplaceMissingView(layerType, timestamp);
        return viewDeltas.at(layerType).at(timestamp);
    }

    UnpackedView<SegmentAtom>& TileViewDeltas::topDown(const time_t timestamp) {
        return deltaView(LayerType::TOP_DOWN, timestamp);
    }

    UnpackedView<SegmentAtom>& TileViewDeltas::roofless(const time_t timestamp) {
        return deltaView(LayerType::TOP_DOWN_ROOFLESS, timestamp);
    }

    UnpackedView<SegmentAtom>& TileViewDeltas::heightmap(const time_t timestamp) {
        return deltaView(LayerType::HEIGHTMAP, timestamp);
    }

    UnpackedView<SegmentAtom>& TileViewDeltas::heightmapRoofless(const time_t timestamp) {
        return deltaView(LayerType::HEIGHTMAP_ROOFLESS, timestamp);
    }

    UnpackedView<SegmentAtom>& TileViewDeltas::drainedTopDown(const time_t timestamp) {
        return deltaView(LayerType::DRAINED_TOP_DOWN, timestamp);
    }

    UnpackedView<SegmentAtom>& TileViewDeltas::drainedTopDownHeightmap(const time_t timestamp) {
        return deltaView(LayerType::DRAINED_TOP_DOWN_HEIGHTMAP, timestamp);
    }

    LayerTable2d TileViewDeltas::createBlockLayers() const {
        LayerTable2d layers{SECTION_2D_SIZE_BLOCKS};
        for (const auto& [layerType, deltas] : viewDeltas) {
            std::vector<PackedSnapshot<SegmentAtom>> reverseDeltas;
            reverseDeltas.reserve(deltas.size());
            PackedDeltaData deltaBlockStates(reverseDeltas, layers.snapshotSize);

            std::vector<time_t> timestamps;
            for (const auto timestamp : deltas | std::views::keys)
                timestamps.push_back(timestamp);

            std::ranges::sort(timestamps, std::greater());

            for (const auto timestamp : timestamps) {
                const auto& view = deltas.at(timestamp);
                const auto _ = deltaBlockStates.insertSnapshot(view.packSnapshot(timestamp));
            }
            layers[layerType] = Layer2d{deltaBlockStates, layerType};
        }
        return layers;
    }

    ZVCR2File convertZVCR3FileToZVCR2File(const ZVCR3File& zvcr3File) {
        const auto& [version, dimensionType, region] = zvcr3File;
        const auto& properties = getProperties(dimensionType);
        return ZVCR2File{ZVCR2_VER_LATEST, dimensionType, convertRegion3dToRegion2d(region, properties)};
    }

    Region2d convertRegion3dToRegion2d(const Region3d& region3d, const DimensionProperties &properties) {
        Region2d region2d;
        for (size_t i = 0; i < SEGMENTS_PER_REGION; ++i)
            region2d.segments.push_back(convertOptSegment3dToOptSegment2d(region3d.segments[i], properties));

        return region2d;
    }

    Option<Segment2d> convertOptSegment3dToOptSegment2d(const Option<Segment3d>& segment3dOpt, const DimensionProperties &properties) {
        return segment3dOpt.andThen([&](const Segment3d& segment) {return convertSegment3dToSegment2d(segment, properties);});
    }

    Segment2d convertSegment3dToSegment2d(const Segment3d& segment3dOpt, const DimensionProperties &properties) {
        const auto sectionCount = static_cast<int8_t>(properties.height / 16);
        TileViewDeltas tileViewDeltas;
        std::vector<time_t> timestamps;
        for (const auto& section : segment3dOpt.blockSections.getSections()) {
            for (const auto& [data, timestamp] : section.reverseDeltas) {
                if (std::ranges::find(timestamps, timestamp) == timestamps.end())
                    timestamps.push_back(timestamp);
            }
        }
        std::unordered_map<time_t, std::unordered_map<int8_t, UnpackedView<SegmentAtom>>> cachedSnapshots;
        cachedSnapshots.reserve(timestamps.size());

        const auto topSectionY = static_cast<int8_t>(sectionCount - 1);

        for (const auto ts : timestamps) {
            for (auto sy = topSectionY; sy >= 0; --sy) {
                const auto& section = segment3dOpt.blockSections.readSection(sy);
                const auto latest = section.latestSnapshot();
                if (!latest.some()) continue;

                const auto& [data, timestamp] = latest.unwrap();

                auto snapshotBuilder = data.unpack();
                cachedSnapshots[timestamp].emplace(sy, UnpackedView {SEGMENT_SIDELENGTH_BLOCKS, snapshotBuilder});

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
                    cachedSnapshots[deltaTimestamp].emplace(sy, UnpackedView {SEGMENT_SIDELENGTH_BLOCKS, snapshotBuilder});
                }
                if (!cachedSnapshots[ts].contains(sy)) {
                    if (const auto snapshot = section.snapshotFrom(ts); snapshot.some())
                        cachedSnapshots[ts].emplace(sy, UnpackedView {SEGMENT_SIDELENGTH_BLOCKS, snapshot->data.unpack()});
                }

                if (renderSegment2dForSectionSnapshot(ts, sy, cachedSnapshots[ts].at(sy), tileViewDeltas))
                    break;
            }
        }
        LayerContainer2d biomeContainer {SECTION_2D_SIZE_BIOMES};
        Layer2d topDownBiomeLayer {biomeContainer.snapshotSize, LayerType::TOP_DOWN};

        const auto& section = segment3dOpt.biomeSections.readSection(topSectionY);
        constexpr auto biomeSegmentTopY = SEGMENT_SIDELENGTH_BIOMES - 1;

        for (const auto& [sectionData, deltaTimestamp] : section.reverseDeltas) {
            const auto sectionView = sectionData.view(SEGMENT_SIDELENGTH_BIOMES);
            auto biomeSectionView = UnpackedView<SegmentAtom>::create2DBiomeView();

            for (uint8_t cx = 0; cx < biomeSectionView.getSidelength(); cx++) {
                for (uint8_t cz = 0; cz < biomeSectionView.getSidelength(); cz++) {
                    biomeSectionView.setPixel(cx, cz, sectionView.getVoxel(cx, biomeSegmentTopY, cz));
                }
            }
            const auto _ = topDownBiomeLayer.deltas.insertSnapshot(biomeSectionView.packSnapshot(deltaTimestamp));
        }
        biomeContainer.setLayer(topDownBiomeLayer.type, topDownBiomeLayer);

        return Segment2d(
            LayerContainer2d {tileViewDeltas.createBlockLayers()},
            biomeContainer,
            segment3dOpt.info
        );
    }

    bool invisibleBlockState(const uint16_t state) {
#if PROTOCOL_VERSION >= 765 && PROTOCOL_VERSION <= 766
        constexpr uint16_t voidAirId = 12958, caveAirId = 12959,
                           barrierId = 10366, airId = 0;
        return state == airId || state == caveAirId || state == voidAirId || state == barrierId;
#else
        return state == 0;
#endif
    }

    bool liquidBlockStateOrLilypad(const uint16_t state) {
#if PROTOCOL_VERSION >= 765 && PROTOCOL_VERSION <= 766
        constexpr uint16_t lowerboundLiquidId = 80, upperboundLiquidId = 111;
        constexpr uint16_t lilypadId = 7271;
        return (state >= lowerboundLiquidId && state <= upperboundLiquidId) || state == lilypadId;
#else
        return false;
#endif
    }

    bool roofBlockType(const uint16_t state) {
#if PROTOCOL_VERSION >= 765 && PROTOCOL_VERSION <= 766
        constexpr uint16_t bedrockId = 79, obsidianId = 2354, cryingObsidianId = 19449;
        constexpr uint16_t lowerboundSnowId = 5772, upperboundSnowId = 5779;
        return state == bedrockId
                 || state == obsidianId
                 || state == cryingObsidianId
                 || (state >= lowerboundSnowId && state <= upperboundSnowId);
#else
        return false;
#endif
    }

    bool renderSegment2d(const uint8_t cx, const uint8_t cz, const uint8_t sy, const UnpackedView<SegmentAtom>& sectionView,
                          UnpackedView<SegmentAtom>& topDownTileView, UnpackedView<SegmentAtom>& heightmapTileView,
                          const bool ignoreRoof, const bool ignoreLiquids) {
        if (const auto current = topDownTileView.getPixel(cx, cz); !invisibleBlockState(current))
            return true;

        for (int8_t cy = SEGMENT_SIDELENGTH_BLOCKS - 1; cy >= 0; --cy) {
            const auto state = sectionView.getVoxel(cx, cy, cz);

            if (invisibleBlockState(state)) continue;
            if (ignoreLiquids && liquidBlockStateOrLilypad(state)) continue;
            if (ignoreRoof && roofBlockType(state)) continue;

            topDownTileView.setPixel(cx, cz, state);

            if (const auto height = sy * 16 + cy; height > heightmapTileView.getPixel(cx, cz)) {
                heightmapTileView.setPixel(cx, cz, height);
                return true;
            }
        }
        return false;
    }

    bool renderSegment2dForSectionSnapshot(const time_t timestamp, const uint8_t sy, const UnpackedView<SegmentAtom>& sectionView,
                                            TileViewDeltas& tileViewDeltas) {

        auto& topDownTileView = tileViewDeltas.topDown(timestamp);
        auto& rooflessTileView = tileViewDeltas.roofless(timestamp);
        auto& heightmapTileView = tileViewDeltas.heightmap(timestamp);
        auto& heightmapRooflessTileView = tileViewDeltas.heightmapRoofless(timestamp);
        auto& drainedTopDownTileView = tileViewDeltas.drainedTopDown(timestamp);
        auto& drainedTopDownHeightmapTileView = tileViewDeltas.drainedTopDownHeightmap(timestamp);

        bool finished = true;

        for (uint8_t cx = 0; cx < SEGMENT_SIDELENGTH_BLOCKS; ++cx) {
            for (uint8_t cz = 0; cz < SEGMENT_SIDELENGTH_BLOCKS; ++cz) {
                finished &= renderSegment2d(cx, cz, sy, sectionView,
                    topDownTileView, heightmapTileView, false, false);

                finished &= renderSegment2d(cx, cz, sy, sectionView,
                    drainedTopDownTileView, drainedTopDownHeightmapTileView, false, true);

                finished &= renderSegment2d(cx, cz, sy, sectionView,
                    rooflessTileView, heightmapRooflessTileView, true, false);
            }
        }
        return finished;
    }
}
