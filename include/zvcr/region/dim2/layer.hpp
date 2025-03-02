#pragma once

#include <cstdint>
#include <unordered_map>
#include <zvcr/common/data_storage.hpp>

namespace zvcr::region::dim2::layer {

    using namespace common::reverse_delta;
    using namespace common::definitions;

    enum class LayerType {
        TOP_DOWN = 0,
        TOP_DOWN_ROOFLESS = 1,
        HEIGHTMAP = 2,
        HEIGHTMAP_ROOFLESS = 3,
        DRAINED_TOP_DOWN = 4,
        DRAINED_TOP_DOWN_HEIGHTMAP = 5,
        CUSTOM
    };

    using LayerTypeId = uint8_t;

    struct Layer2d {
        PackedDeltaData<Segment2dAtom> deltas;
        LayerTypeId type{};

        explicit Layer2d(const PackedDeltaData<Segment2dAtom>& deltas, const LayerTypeId type):
            deltas(deltas), type(type) {}

        explicit Layer2d(const size_t snapshotSize, const LayerTypeId type):
            deltas(snapshotSize), type(type) {}

        explicit Layer2d(const size_t snapshotSize, const LayerType type):
            deltas(snapshotSize), type(static_cast<LayerTypeId>(type)) {}

        explicit Layer2d(const LayerTypeId type):
            deltas(SECTION_2D_SIZE_BLOCKS), type(type) {}

        explicit Layer2d(const LayerType type):
            deltas(SECTION_2D_SIZE_BLOCKS), type(static_cast<LayerTypeId>(type)) {}
    };

    class LayerContainer2d : public std::unordered_map<LayerTypeId, Layer2d> {
    public:
        [[nodiscard]]
        Layer2d& operator[](LayerType layerType);

        [[nodiscard]]
        Layer2d& operator[](LayerTypeId layerType);
    };
}
