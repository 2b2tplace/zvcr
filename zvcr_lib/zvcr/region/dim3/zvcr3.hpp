#pragma once

#include <zvcr/region/dimension.hpp>
#include <zvcr/region/dim3/segment3.hpp>

namespace zvcr {

    enum class ZVCR3Version {
        ZVCR3_0_0_0_1 = 1,
        ZVCR3_0_1_0_0,
        ZVCR3_0_1_1_0,
        ZVCR3_0_1_2_0
    };

    static constexpr auto ZVCR3_VER_LATEST = ZVCR3Version::ZVCR3_0_1_2_0;
    static constexpr auto ZVCR3_FILE_PREFIX = "ZVRegion";

    struct ZVCR3File {
        ZVCR3Version version{ZVCR3_VER_LATEST};
        DimensionType dimensionType;
        Region3d region;

        explicit ZVCR3File(const ZVCR3Version version, const DimensionType dimensionType, Region3d region):
            version(version),
            dimensionType(dimensionType),
            region(std::move(region)) {}

        explicit ZVCR3File(const DimensionType dimensionType, Region3d region):
            ZVCR3File(ZVCR3_VER_LATEST, dimensionType, std::move(region)) {}

        ZVCR3File(ZVCR3File &&other) noexcept:
            version(other.version),
            dimensionType(other.dimensionType),
            region(std::move(other.region)) {}

        ZVCR3File(const ZVCR3File &other) = default;
    };

}
