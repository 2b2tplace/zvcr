#pragma once

#include <utility>
#include <zvcr/region/dimension.hpp>
#include <zvcr/region/dim2/segment2.hpp>

namespace zvcr {

    enum class ZVCR2Version {
        ZVCR2_0_0_0_0 = 0,
        ZVCR2_0_1_0_0,
        ZVCR2_0_1_1_0,
        ZVCR2_0_1_1_1,
        ZVCR2_0_1_2_0,
        ZVCR2_0_1_3_0
    };

    static constexpr auto ZVCR2_VER_LATEST = ZVCR2Version::ZVCR2_0_1_3_0;
    static constexpr auto ZVCR2_FILE_PREFIX = "ZPRegion";

    struct ZVCR2File {
        ZVCR2Version version{ZVCR2_VER_LATEST};
        DimensionType dimensionType;
        Region2d region;

        explicit ZVCR2File(const ZVCR2Version version, const DimensionType dimensionType, Region2d region):
            version(version),
            dimensionType(dimensionType),
            region(std::move(region)) {}

        explicit ZVCR2File(const DimensionType dimensionType, Region2d region):
            ZVCR2File(ZVCR2_VER_LATEST, dimensionType, std::move(region)) {}

        ZVCR2File(ZVCR2File &&other) noexcept:
            version(other.version),
            dimensionType(other.dimensionType),
            region(std::move(other.region)) {}

        ZVCR2File(const ZVCR2File &other) = default;
    };

}