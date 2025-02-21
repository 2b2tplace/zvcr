#pragma once

#include <zvcr/region/dimension.hpp>
#include <zvcr/region/dim3/segment3.hpp>

namespace zvcr::region::dim3::zvcr3 {

    enum class ZVCR3Version {
        ZVCR3_0_0_0_1 = 1
    };

    constexpr static auto ZVCR3_VER_LATEST = ZVCR3Version::ZVCR3_0_0_0_1;
    constexpr static auto ZVCR3_FILE_PREFIX = "ZVRegion";

    struct ZVCR3File {
        ZVCR3Version version{};
        dimension::DimensionType dimensionType{};
        segment3::Region3d region;
    };

}
