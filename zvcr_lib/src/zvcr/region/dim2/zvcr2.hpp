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
        ZVCR2_0_1_3_0,
        ZVCR2_0_1_4_0
    };

    static constexpr std::array<std::string_view, 7> versionNames2 {
        "0.0.0.0",
        "0.1.0.0",
        "0.1.1.0",
        "0.1.1.1",
        "0.1.2.0",
        "0.1.3.0",
        "0.1.4.0"
    };

    [[nodiscard]]
    constexpr auto versionName2(const ZVCR2Version version) -> std::string_view {
        return versionNames2[static_cast<size_t>(version)];
    }

    static constexpr auto ZVCR2_VER_LATEST = ZVCR2Version::ZVCR2_0_1_4_0;
    static constexpr auto ZVCR2_FILE_PREFIX = "ZPRegion";

    struct ZVCR2File {
        ZVCR2Version version{ZVCR2_VER_LATEST}; // changing this has no effect during serialization (backwards compatibility only for deserialization)
        DimensionType dimensionType;
        Region2d region;

        explicit ZVCR2File(const ZVCR2Version version, const DimensionType dimensionType, const uint16_t protocolVersion):
            version(version),
            dimensionType(dimensionType),
            region(protocolVersion) {}

        explicit ZVCR2File(const DimensionType dimensionType, const uint16_t protocolVersion):
            ZVCR2File(ZVCR2_VER_LATEST, dimensionType, protocolVersion) {}

        ZVCR2File(ZVCR2File &&other) noexcept:
            version(other.version),
            dimensionType(other.dimensionType),
            region(std::move(other.region)) {}

        ZVCR2File(const ZVCR2File &other) = default;
    };

}