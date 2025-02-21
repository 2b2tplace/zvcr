#pragma once

#include <cstdint>
#include <unordered_map>
#include <zvcr/common/reverse_delta.hpp>

namespace zvcr::region::dim2::layer {

    using namespace common::reverse_delta;

    enum class LayerType {
        TOP_DOWN = 0,
        TOP_DOWN_ROOFLESS = 1,
        HEIGHTMAP = 2,
        HEIGHTMAP_ROOFLESS = 3,
        DRAINED_TOP_DOWN = 4,
        DRAINED_TOP_DOWN_HEIGHTMAP = 5,
        CUSTOM
    };

    struct Layer2d {
        DeltaBlockStates deltas;
        uint8_t type{};
    };

    class Layers2d : public std::unordered_map<uint8_t, Layer2d> {
    public:
        [[nodiscard]]
        Layer2d& operator[](LayerType layerType);

        [[nodiscard]]
        Layer2d& operator[](uint8_t layerType);
    };
}