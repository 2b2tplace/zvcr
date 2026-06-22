#pragma once

#include <zvcr/region/version.hpp>
#include <zvcr/dimension.hpp>
#include <cstdint>

namespace zvcr {

    inline constexpr uint16_t PROTOCOL_VERSION_ZVCR_0_0_0_X = 765; // 1.20.4

    struct Context {
        size_t sectionCount{};
        bool supportBiomes{};
        bool supportDynamicVersioning{};
        bool legacyVersion{};
        bool supportSingleValuePalette{};
        bool supportTileEntities{};
        uint16_t protocolVersion{};

        auto initializeSectionCount(const DimensionType dimensionType) -> void {
            sectionCount = dimensionSectionCount(dimensionType);
        }

        auto initialize(const Version version) -> void {
            supportBiomes = version >= Version::ZVCR3D_0_1_0_0;
            supportDynamicVersioning = version >= Version::ZVCR3D_0_1_1_0;
            supportSingleValuePalette = version >= Version::ZVCR3D_0_1_3_0;
            supportTileEntities = version >= Version::ZVCR3D_0_1_4_0;
            legacyVersion = version <= Version::ZVCR3D_0_1_4_0; // support removed in future versions

            if (protocolVersion == 0 && version == Version::ZVCR3D_0_0_0_1)
                protocolVersion = PROTOCOL_VERSION_ZVCR_0_0_0_X;
        }
    };

}