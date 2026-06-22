#pragma once

#include <zvcr/io/file_type.hpp>
#include <zvcr/dimension.hpp>
#include <result.hpp>

namespace zvcr {

    using RegionID = uint64_t;
    inline constexpr int32_t SECTOR_SIDELENGTH = 32;

    constexpr int32_t floorDivSector(const int32_t rc) {
        return rc >= 0 ? rc / SECTOR_SIDELENGTH : (rc - SECTOR_SIDELENGTH + 1) / SECTOR_SIDELENGTH;
    }

    struct RegionLocation {
        int32_t rx;
        int32_t rz;
        DimensionType dimensionType;

        [[nodiscard]]
        auto toRegionID() const -> RegionID;

        [[nodiscard]]
        static auto fromRegionID(RegionID regionID) -> RegionLocation;

        [[nodiscard]]
        static auto fileExtensionLength(const std::string &filename) -> result::Option<size_t>;

        [[nodiscard]]
        static auto fromFileName(DimensionType dimension, const fs::path &file) -> result::Option<RegionLocation>;

        [[nodiscard]]
        auto directory(const fs::path &parentDirectory) const -> fs::path;

        [[nodiscard]]
        auto fileNameExtensionless() const -> std::string;

        [[nodiscard]]
        auto fileName(std::string_view extension) const -> std::string;

        [[nodiscard]]
        auto filePath(const fs::path &parentDirectory, std::string_view extension) const -> fs::path;

        [[nodiscard]]
        auto fileName() const -> std::string {
            return fileName(extension);
        }

        [[nodiscard]]
        auto filePath(const fs::path &parentDirectory) const -> fs::path {
            return filePath(parentDirectory, extension);
        }
    };

}
