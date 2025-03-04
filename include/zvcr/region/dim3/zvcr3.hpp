#pragma once

#include <zvcr/region/dimension.hpp>
#include <zvcr/region/dim3/segment3.hpp>

namespace zvcr::region::dim3::zvcr3 {

    enum class ZVCR3Version {
        ZVCR3_0_0_0_1 = 1,
        ZVCR3_0_1_0_0 = 2
    };

    constexpr static auto ZVCR3_VER_LATEST = ZVCR3Version::ZVCR3_0_1_0_0;
    constexpr static auto ZVCR3_FILE_PREFIX = "ZVRegion";

    struct ZVCR3File {
        ZVCR3Version version {ZVCR3_VER_LATEST};
        dimension::DimensionType dimensionType;
        segment3::Region3d region;

        explicit ZVCR3File(const ZVCR3Version version, const dimension::DimensionType dimensionType, segment3::Region3d region):
            version(version),
            dimensionType(dimensionType),
            region(std::move(region)) {}

        explicit ZVCR3File(const dimension::DimensionType dimensionType, const segment3::Region3d &region):
            ZVCR3File(ZVCR3_VER_LATEST, dimensionType, region) {}
    };

}
