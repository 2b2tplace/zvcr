#pragma once

#include <filesystem>
#include <zvcr/region/dimension.hpp>
#include <result.hpp>

namespace zvcr {

    namespace fs = std::filesystem;

    using RegionID = uint64_t;
    static constexpr int32_t SECTOR_SIDELENGTH = 32;

    const std::string ZVCR_REGION_PREFIX = "r.";
    const std::string ZVCR_REGION_DELIMITER = ".";
    const std::string ZVCR_EXTENSION = ".zvcr";

    enum class RegionFormat {
        ZVCR2 = 2,
        ZVCR3 = 3
    };

    struct RegionLocation {
        int32_t rx;
        int32_t rz;
        DimensionType dimensionType;

        [[nodiscard]]
        auto toRegionID() const -> RegionID;

        [[nodiscard]]
        static auto fromRegionID(RegionID regionID) -> RegionLocation;

        [[nodiscard]]
        static auto fromFileName(DimensionType dimension, const fs::path& file) -> result::Option<RegionLocation>;

        [[nodiscard]]
        auto getDirectory(const fs::path& parentDirectory) const -> fs::path;

        [[nodiscard]]
        auto getFileName(RegionFormat format) const -> std::string;

        [[nodiscard]]
        auto getFilePath(const fs::path& parentDirectory, RegionFormat format) const -> fs::path;
    };

}
