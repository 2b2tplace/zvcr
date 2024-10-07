#pragma once

#include <libzr/modules/common/zr_dimension.hpp>
#include <libzr/modules/zvr/zvr_chunk.hpp>

enum class ZvrVersion {
    ZVR_BETA_1_0_0_R0 = 0, // unsupported; dropped backwards compatibility
    ZVR_BETA_1_0_1_R0 = 1
};

#define ZVR_LATEST ZvrVersion::ZVR_BETA_1_0_1_R0
#define ZVR_PREFIX "ZVRegion"

struct ZvrFile {
    ZvrVersion version;
    ZrDimensionType dimensionType;
    ZvrRegion region;
};