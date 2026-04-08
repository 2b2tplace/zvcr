#pragma once

#include <unordered_map>
#include <array>

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

    static constexpr std::array<std::string_view, 3> dimensionNames {
        "overworld",
        "nether",
        "end"
    };

    static const std::unordered_map<DimensionType, DimensionProperties> DimensionTypePropertyRegistry = {
        {DimensionType::OVERWORLD, DimensionProperties {true, -64, 384}},
        {DimensionType::NETHER, DimensionProperties {false, 0, 256}},
        {DimensionType::THE_END, DimensionProperties {false, 0, 256}}
    };

    [[nodiscard]]
    inline auto getProperties(const DimensionType type) -> const DimensionProperties& {
        return DimensionTypePropertyRegistry.at(type);
    }

    [[nodiscard]]
    constexpr auto dimensionName(const DimensionType type) -> std::string_view {
        return dimensionNames[static_cast<size_t>(type)];
    }

}