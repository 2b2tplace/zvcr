#pragma once

#include <utility>
#include <zvcr/region/dimension.hpp>
#include <zvcr/region/dim2/segment2.hpp>

namespace zvcr::region::dim2::zvcr2 {

    enum class ZVCR2Version {
        ZVCR2_0_0_0_0 = 0,
        ZVCR2_0_1_0_0 = 1
    };

    constexpr static auto ZVCR2_VER_LATEST = ZVCR2Version::ZVCR2_0_1_0_0;
    constexpr static auto ZVCR2_FILE_PREFIX = "ZPRegion";

    struct ZVCR2File {
        ZVCR2Version version {ZVCR2_VER_LATEST};
        dimension::DimensionType dimensionType;
        segment2::Region2d region;

        explicit ZVCR2File(const ZVCR2Version version, const dimension::DimensionType dimensionType, segment2::Region2d region):
            version(version),
            dimensionType(dimensionType),
            region(std::move(region)) {}

        explicit ZVCR2File(const dimension::DimensionType dimensionType, const segment2::Region2d &region):
            ZVCR2File(ZVCR2_VER_LATEST, dimensionType, region) {}
    };

}