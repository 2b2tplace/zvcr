#pragma once

#include <libzr/modules/common/zr_dimension.hpp>
#include <libzr/modules/zpr/zpr_segment.hpp>

enum class ZprVersion {
    ZPR_BETA_1_0_0_R0 = 0
};

#define ZPR_LATEST ZprVersion::ZPR_BETA_1_0_0_R0
#define ZPR_PREFIX "ZPRegion"

struct ZprFile {
    ZprVersion version;
    ZrDimensionType dimensionType;
    ZprRegion region;
};