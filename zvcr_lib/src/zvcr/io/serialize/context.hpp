#pragma once

#include <zvcr/region/version.hpp>
#include <zvcr/dimension.hpp>
#include <cstdint>

namespace zvcr {

    inline constexpr uint16_t PROTOCOL_VERSION_ZVCR_0_0_0_X = 765; // 1.20.4

    struct Context {
        size_t sectionCount{};
        uint16_t protocolVersion{};

        auto initializeSectionCount(const DimensionType dimensionType) -> void {
            sectionCount = dimensionSectionCount(dimensionType);
        }

        auto initialize(const Version version) -> void {
            // for future reference: add version checks here for backwards-compatible deserialize
        }
    };

}