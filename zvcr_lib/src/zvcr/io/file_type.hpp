#pragma once

#include <filesystem>
#include <zvcr/dimension.hpp>
#include <zvcr/region/version.hpp>
#include <zvcr/region/segment.hpp>

namespace zvcr {

#ifdef PROTOCOL_VERSION
    inline constexpr uint16_t DEFAULT_PROTOCOL_VERSION = PROTOCOL_VERSION;
#else
    inline constexpr uint16_t DEFAULT_PROTOCOL_VERSION = 769;
#endif

    inline constexpr std::string_view ZVCR_REGION_PREFIX = "r.";

    namespace fs = std::filesystem;

    struct File {
        Version version{ZVCR3D_LATEST_VERSION};
        uint16_t protocolVersion{};
        DimensionType dimensionType{};
        Region region{DEFAULT_PROTOCOL_VERSION};
    };

    inline constexpr std::string_view extension = "zvcr3d";
    inline constexpr std::string_view filePrefix = extension;
    inline constexpr size_t preallocate = 32 * 1024 * 1024;
}