#pragma once

#include <zvcr/definitions.hpp>
#include <unordered_map>
#include <array>
#include <cstdint>
#include <string_view>

namespace zvcr {

    struct DimensionProperties {
        bool hasSkyLight;
        int32_t minY;
        uint32_t height;
    };

    enum class DimensionType {
        OVERWORLD = 0,
        NETHER = 1,
        THE_END = 2
    };

    struct Dimension {
        DimensionType type;
        DimensionProperties properties;
    };

    inline constexpr std::array<std::string_view, 3> DIMENSION_NAMES {
        "overworld",
        "nether",
        "end"
    };

    static const std::unordered_map<DimensionType, DimensionProperties> DIMENSION_PROPERTIES = {
        {DimensionType::OVERWORLD, DimensionProperties {true, -64, 384}},
        {DimensionType::NETHER, DimensionProperties {false, 0, 256}},
        {DimensionType::THE_END, DimensionProperties {false, 0, 256}}
    };

    [[nodiscard]]
    inline auto dimensionProperties(const DimensionType type) -> const DimensionProperties& {
        return DIMENSION_PROPERTIES.at(type);
    }

    [[nodiscard]]
    constexpr auto dimensionName(const DimensionType type) -> std::string_view {
        return DIMENSION_NAMES[static_cast<size_t>(type)];
    }

    [[nodiscard]]
    inline auto dimensionMinSectionY(const DimensionType type) -> int32_t {
        return dimensionProperties(type).minY / SEGMENT_SIDELENGTH_BLOCKS;
    }

    [[nodiscard]]
    inline auto dimensionSectionCount(const DimensionType type) -> size_t {
        return dimensionProperties(type).height / SEGMENT_SIDELENGTH_BLOCKS;
    }

}
