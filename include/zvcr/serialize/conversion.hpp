#pragma once

#include <cstdint>
#include <ctime>
#include <unordered_map>
#include <zvcr/region/dim2/layer.hpp>
#include <zvcr/region/dim2/segment2.hpp>
#include <zvcr/region/dim2/zvcr2.hpp>
#include <zvcr/region/dim3/zvcr3.hpp>

namespace zvcr::serialize::conversion {

    using namespace region::dim2::layer;
    using namespace region::dim2::segment2;
    using namespace region::dim2::zvcr2;
    using namespace region::dim3::segment3;
    using namespace region::dim3::zvcr3;
    using region::dimension::DimensionProperties;
    using common::paletted_storage::UnpackedView;

    class TileViewDeltas {
    public:
        TileViewDeltas() = default;

        void emplaceMissingView(uint8_t layerType, time_t timestamp);

        [[nodiscard]]
        UnpackedView<SegmentAtom>& deltaView(LayerType layerType, time_t timestamp);

        [[nodiscard]]
        UnpackedView<SegmentAtom>& deltaView(uint8_t layerType, time_t timestamp);

        [[nodiscard]]
        UnpackedView<SegmentAtom>& topDown(time_t timestamp);

        [[nodiscard]]
        UnpackedView<SegmentAtom>& roofless(time_t timestamp);

        [[nodiscard]]
        UnpackedView<SegmentAtom>& heightmap(time_t timestamp);

        [[nodiscard]]
        UnpackedView<SegmentAtom>& heightmapRoofless(time_t timestamp);

        [[nodiscard]]
        UnpackedView<SegmentAtom>& drainedTopDown(time_t timestamp);

        [[nodiscard]]
        UnpackedView<SegmentAtom>& drainedTopDownHeightmap(time_t timestamp);

        [[nodiscard]]
        LayerTable2d createBlockLayers() const;
    private:
        std::unordered_map<uint8_t, std::unordered_map<time_t, UnpackedView<SegmentAtom>>> viewDeltas;
    };

    [[nodiscard]]
    ZVCR2File convertZVCR3FileToZVCR2File(const ZVCR3File& zvcr3File);

    [[nodiscard]]
    Region2d convertRegion3dToRegion2d(const Region3d& region3d, const DimensionProperties& properties);

    [[nodiscard]]
    Option<Segment2d> convertOptSegment3dToOptSegment2d(const Option<Segment3d>& segment3dOpt, const DimensionProperties& properties);

    [[nodiscard]]
    Segment2d convertSegment3dToSegment2d(const Segment3d& segment3dOpt, const DimensionProperties& properties);

    [[nodiscard]]
    bool renderSegment2dForSectionSnapshot(time_t timestamp, uint8_t sy, const UnpackedView<SegmentAtom>& sectionView, TileViewDeltas& tileViewDeltas);
}
