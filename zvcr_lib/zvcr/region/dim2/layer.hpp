#pragma once

#include <unordered_map>
#include <utility>
#include <zvcr/common/data_storage.hpp>

namespace zvcr {

    enum class LayerType {
        NORMAL = 0,
        DRAINED = 4,
        TERRAIN = 1,
        DRAINED_TERRAIN = 7,
        HEIGHTMAP = 2,
        DRAINED_HEIGHTMAP = 5,
        TERRAIN_HEIGHTMAP = 3,
        DRAINED_TERRAIN_HEIGHTMAP = 8,
        PREDICTED = 6,
        CUSTOM = 255
    };

    using LayerTypeId = uint8_t;

    struct Layer2d {
        PackedDeltaData deltas;
        LayerTypeId type{};

        explicit Layer2d(PackedDeltaData deltas, const LayerTypeId type):
            deltas(std::move(deltas)),
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
