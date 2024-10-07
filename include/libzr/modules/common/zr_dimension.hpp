#pragma once

#include <libzr/modules/zr_common.hpp>

struct ZvrDimensionProperties {
    bool hasSkyLight;
    int32_t minY;
    uint32_t height;
};

enum class ZrDimensionType {
    OVERWORLD = 0,
    NETHER = 1,
    THE_END = 2
};

struct ZvrDimension {
    ZrDimensionType type;
    ZvrDimensionProperties properties;
};

static const std::unordered_map<ZrDimensionType, ZvrDimensionProperties> DimensionTypePropertyRegistry = {
    {ZrDimensionType::OVERWORLD, ZvrDimensionProperties {true, -64, 384}},
    {ZrDimensionType::NETHER, ZvrDimensionProperties {false, 0, 256}},
    {ZrDimensionType::THE_END, ZvrDimensionProperties {false, 0, 256}}
};

static const std::unordered_map<ZrDimensionType, std::string> DimensionTypeRegistry = {
    {ZrDimensionType::OVERWORLD, "overworld"},
    {ZrDimensionType::NETHER, "nether"},
    {ZrDimensionType::THE_END, "the end"}
};

std::ostream& operator<<(std::ostream& os, ZrDimensionType dimensionType);