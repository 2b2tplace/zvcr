#pragma once

#include <array>
#include <zvcr/region/dimension.hpp>
#include <zvcr/region/dim3/segment3.hpp>

namespace zvcr {

    enum class ZVCR3Version {
        ZVCR3_0_0_0_1 = 1,
        ZVCR3_0_1_0_0,
        ZVCR3_0_1_1_0,
        ZVCR3_0_1_2_0,
        ZVCR3_0_1_3_0,
        ZVCR3_0_1_4_0
    };

    static constexpr auto ZVCR3_VER_LATEST = ZVCR3Version::ZVCR3_0_1_4_0;
    static constexpr auto ZVCR3_FILE_PREFIX = "ZVRegion";

    static constexpr std::array<std::string_view, 7> versionNames3 {
        "0.0.0.0",
        "0.0.0.1",
        "0.1.0.0",
        "0.1.1.0",
        "0.1.2.0",
        "0.1.3.0",
        "0.1.4.0"
    };

    [[nodiscard]]
    constexpr std::string_view versionName3(const ZVCR3Version version) {
        return versionNames3[static_cast<size_t>(version)];
    }

    struct ZVCR3File {
        ZVCR3Version version{ZVCR3_VER_LATEST}; // changing this has no effect during serialization (backwards compatibility only for deserialization)
        DimensionType dimensionType;
        Region3d region;

        explicit ZVCR3File(const ZVCR3Version version, const DimensionType dimensionType, const uint16_t protocolVersion):
            version(version),
            dimensionType(dimensionType),
            region(protocolVersion) {}

        explicit ZVCR3File(const DimensionType dimensionType, const uint16_t protocolVersion):
            ZVCR3File(ZVCR3_VER_LATEST, dimensionType, protocolVersion) {}

        ZVCR3File(ZVCR3File &&other) noexcept:
            version(other.version),
            dimensionType(other.dimensionType),
            region(std::move(other.region)) {}

        ZVCR3File(const ZVCR3File &other) = default;
    };

}
