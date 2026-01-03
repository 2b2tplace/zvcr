#pragma once

#include <unordered_map>

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
    inline const DimensionProperties& getProperties(const DimensionType type) {
        return DimensionTypePropertyRegistry.at(type);
    }

    [[nodiscard]]
    constexpr std::string_view dimensionName(const DimensionType type) {
        return dimensionNames[std::to_underlying(type)];
    }

}