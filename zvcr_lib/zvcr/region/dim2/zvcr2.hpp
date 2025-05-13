#pragma once

#include <utility>
#include <zvcr/region/dimension.hpp>
#include <zvcr/region/dim2/segment2.hpp>

namespace zvcr::region {

    enum class ZVCR2Version {
        ZVCR2_0_0_0_0 = 0,
        ZVCR2_0_1_0_0 = 1
    };

    static constexpr auto ZVCR2_VER_LATEST = ZVCR2Version::ZVCR2_0_1_0_0;
    static constexpr auto ZVCR2_FILE_PREFIX = "ZPRegion";

    struct ZVCR2File {
        ZVCR2Version version{ZVCR2_VER_LATEST};
        DimensionType dimensionType;
        Region2d region;

        explicit ZVCR2File(const ZVCR2Version version, const DimensionType dimensionType, Region2d region):
            version(version),
            dimensionType(dimensionType),
            region(std::move(region)) {}

        explicit ZVCR2File(const DimensionType dimensionType, const Region2d &region):
            ZVCR2File(ZVCR2_VER_LATEST, dimensionType, region) {}
    };

}