#pragma once

#include <unordered_map>
#include <zvcr/common/data_storage.hpp>

namespace zvcr::region {

    using namespace reverse_delta;
    using namespace definitions;

    enum class LayerType {
        NORMAL = 0,
        TERRAIN = 1,
        HEIGHTMAP = 2,
        TERRAIN_HEIGHTMAP = 3,
        DRAINED = 4,
        DRAINED_HEIGHTMAP = 5,
        PREDICTED = 6,
        CUSTOM = 255
    };

    using LayerTypeId = uint8_t;

    struct Layer2d {
        PackedDeltaData<SegmentAtom> deltas;
        LayerTypeId type{};

        explicit Layer2d(const PackedDeltaData<SegmentAtom>& deltas, const LayerTypeId type):
            deltas(deltas),
            type(type) {}

        explicit Layer2d(const size_t snapshotSize, const LayerTypeId type):
            deltas(snapshotSize),
            type(type) {}

        explicit Layer2d(const size_t snapshotSize, const LayerType type):
            deltas(snapshotSize),
            type(static_cast<LayerTypeId>(type)) {}
    };

    class LayerTable2d : public std::unordered_map<LayerTypeId, Layer2d> {
    public:
        size_t snapshotSize;

        explicit LayerTable2d(const size_t snapshotSize):
            snapshotSize(snapshotSize) {}

        [[nodiscard]]
        Layer2d& operator[](LayerType layerType);

        [[nodiscard]]
        Layer2d& operator[](LayerTypeId layerType);
    };
}
