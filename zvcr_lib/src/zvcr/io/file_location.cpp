#include <zvcr/io/file_location.hpp>
#include <fmt/core.h>
#include <utility>

namespace zvcr {

    auto RegionLocation::toRegionID() const -> RegionID {
        const auto ux = static_cast<uint64_t>(static_cast<uint32_t>(rx) & 0x1FFFFF);
        const auto uz = static_cast<uint64_t>(static_cast<uint32_t>(rz) & 0x1FFFFF);
        const auto ud = static_cast<uint64_t>(dimensionType);

        return ud << 42 | ux << 21 | uz;
    }

    auto RegionLocation::fromRegionID(const RegionID regionID) -> RegionLocation {
        const auto ux = static_cast<int32_t>(regionID >> 21 & 0x1FFFFF);
        const auto uz = static_cast<int32_t>(regionID & 0x1FFFFF);
        const auto ud = static_cast<int32_t>(regionID >> 42 & 0xFFFFF);

        const auto recoveredX = ux >= 0x100000 ? ux - 0x200000 : ux;
        const auto recoveredZ = uz >= 0x100000 ? uz - 0x200000 : uz;
        const auto recoveredDim = static_cast<DimensionType>(ud);

        return RegionLocation{recoveredX, recoveredZ, recoveredDim};
    }

    auto RegionLocation::fileExtensionLength(const std::string &filename) -> result::Option<size_t> {
        if (filename.ends_with(".zvcr3d"))
            return 7;

        if (filename.ends_with(".zvcr3"))
            return 6;

        if (filename.ends_with(".zvr"))
            return 4;

        return {};
    }

    auto RegionLocation::fromFileName(const DimensionType dimension, const fs::path &file) -> result::Option<RegionLocation> {
        const auto filename = file.filename().string();
        if (!filename.starts_with(ZVCR_REGION_PREFIX)) return result::None;

        const auto extensionLength = REQUIRE(fileExtensionLength(filename));

        static constexpr auto startOffset = ZVCR_REGION_PREFIX.length();
        const auto endOffset = extensionLength + startOffset;

        const auto regionIdentifier = filename.substr(startOffset, filename.length() - endOffset);
        const auto delimiter = regionIdentifier.find_first_of('.');
        if (delimiter == std::string::npos) return result::None;

        const auto regionX = std::stoi(regionIdentifier.substr(0, delimiter));
        const auto regionZ = std::stoi(regionIdentifier.substr(delimiter + 1));

        return RegionLocation{regionX, regionZ, dimension};
    }

    auto RegionLocation::directoryLegacy(const fs::path &parentDirectory) const -> fs::path {
        const auto sectorX = std::to_string(rx / SECTOR_SIDELENGTH);
        const auto sectorZ = std::to_string(rz / SECTOR_SIDELENGTH);
        const auto dimID = std::to_string(std::to_underlying(dimensionType));

        return fs::path(parentDirectory) / dimID / sectorX / sectorZ;
    }

    auto RegionLocation::directory(const fs::path &parentDirectory) const -> fs::path {
        const auto sectorX = std::to_string(floorDivSector(rx));
        const auto sectorZ = std::to_string(floorDivSector(rz));
        const auto dimID = dimensionName(dimensionType);

        return fs::path(parentDirectory) / dimID / sectorX / sectorZ;
    }

    auto RegionLocation::fileNameExtensionless() const -> std::string {
        return fmt::format("{}{}.{}", ZVCR_REGION_PREFIX, rx, rz);
    }

    auto RegionLocation::fileName(const std::string_view extension) const -> std::string {
        return fmt::format("{}.{}", fileNameExtensionless(), extension);
    }

    auto RegionLocation::filePath(const fs::path &parentDirectory, const std::string_view extension) const -> fs::path {
        return fs::path(directory(parentDirectory)) / fileName(extension);
    }

}
