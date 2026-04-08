#pragma once

#include <unordered_map>
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

    [[nodiscard]]
    auto getHeightmapLayer(LayerType topDownLayer) -> LayerType;

    template<size_t snapshotLength>
    struct Layer2d {
        PackedDeltaData<snapshotLength> deltas;
        LayerTypeId type{};

        explicit Layer2d(PackedDeltaData<snapshotLength> deltas, const LayerTypeId type):
            deltas(std::move(deltas)),
            type(type) {}

        explicit Layer2d(const LayerTypeId type):
            deltas(),
            type(type) {}

        explicit Layer2d(const LayerType type):
            deltas(),
            type(static_cast<LayerTypeId>(type)) {}
    };

    template<size_t snapshotLength>
    class LayerTable2d : public std::unordered_map<LayerTypeId, Layer2d<snapshotLength>> {
        using Map = std::unordered_map<LayerTypeId, Layer2d<snapshotLength>>;
    public:
        LayerTable2d() = default;

        [[nodiscard]]
        auto operator[](LayerType layerType) -> Layer2d<snapshotLength>& {
            return operator[](static_cast<LayerTypeId>(layerType));
        }

        [[nodiscard]]
        auto operator[](LayerTypeId layerType) -> Layer2d<snapshotLength>& {
            if (Map::contains(layerType))
                return Map::at(layerType);

            const auto deltas = PackedDeltaData{PackedSnapshotVector<snapshotLength>{}};
            const auto emptyLayer = Layer2d{deltas, layerType};
            Map::emplace(layerType, emptyLayer);

            return Map::at(layerType);
        }
    };
}
