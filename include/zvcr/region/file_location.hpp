#pragma once

#include <filesystem>
#include <zvcr/region/dimension.hpp>

namespace zvcr::region {

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
        RegionID toRegionID() const;

        [[nodiscard]]
        static RegionLocation fromRegionID(RegionID regionID);

        [[nodiscard]]
        fs::path getDirectory(const std::string& parentDirectory) const;

        [[nodiscard]]
        std::string getFileName(RegionFormat format) const;

        [[nodiscard]]
        fs::path getFilePath(const fs::path& parentDirectory, RegionFormat format) const;
    };

}
