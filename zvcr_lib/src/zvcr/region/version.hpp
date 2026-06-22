#pragma once

#include <string_view>
#include <array>

namespace zvcr {

    enum class Version {
        ZVCR3D_0_0_0_1 = 1,
        ZVCR3D_0_1_0_0,
        ZVCR3D_0_1_1_0,
        ZVCR3D_0_1_2_0,
        ZVCR3D_0_1_3_0,
        ZVCR3D_0_1_4_0,
        ZVCR3D_1_0_0_0
    };

    inline constexpr auto ZVCR3D_LATEST_VERSION = Version::ZVCR3D_1_0_0_0;

    inline constexpr std::array<std::string_view, 8> ZVCR3D_VERSION_NAMES {
        "0.0.0.0",
        "0.0.0.1",
        "0.1.0.0",
        "0.1.1.0",
        "0.1.2.0",
        "0.1.3.0",
        "0.1.4.0",
        "1.0.0.0"
    };

    [[nodiscard]]
    constexpr auto zvcr3dVersionName(const Version version) -> std::string_view {
        return ZVCR3D_VERSION_NAMES[static_cast<size_t>(version)];
    }

}
