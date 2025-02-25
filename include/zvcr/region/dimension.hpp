#pragma once

#include <string>
#include <unordered_map>

namespace zvcr::region::dimension {

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

    static const std::unordered_map<DimensionType, DimensionProperties> DimensionTypePropertyRegistry = {
        {DimensionType::OVERWORLD, DimensionProperties {true, -64, 384}},
        {DimensionType::NETHER, DimensionProperties {false, 0, 256}},
        {DimensionType::THE_END, DimensionProperties {false, 0, 256}}
    };

    static const std::unordered_map<DimensionType, std::string> DimensionTypeToString = {
        {DimensionType::OVERWORLD, "overworld"},
        {DimensionType::NETHER, "nether"},
        {DimensionType::THE_END, "the end"}
    };

    [[nodiscard]]
    inline const std::string& toString(const DimensionType type) {
        return DimensionTypeToString.at(type);
    }

    [[nodiscard]]
    inline const DimensionProperties& getProperties(const DimensionType type) {
        return DimensionTypePropertyRegistry.at(type);
    }

}
