#pragma once

#include <zvcr/region/dimension.hpp>
#include <zvcr/region/dim2/segment2.hpp>

namespace zvcr::region::dim2::zvcr2 {

    enum class ZVCR2Version {
        ZVCR2_0_0_0_0 = 0
    };

    constexpr static auto ZVCR2_VER_LATEST = ZVCR2Version::ZVCR2_0_0_0_0;
    constexpr static auto ZVCR2_FILE_PREFIX = "ZPRegion";

    struct ZVCR2File {
        ZVCR2Version version{};
        dimension::DimensionType dimensionType{};
        segment2::Region2d region;
    };

}