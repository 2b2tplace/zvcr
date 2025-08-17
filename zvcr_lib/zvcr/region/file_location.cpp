#include <zvcr/region/file_location.hpp>

namespace zvcr {

    RegionID RegionLocation::toRegionID() const {
        const auto ux = static_cast<uint64_t>(static_cast<uint32_t>(rx) & 0x1FFFFF);
        const auto uz = static_cast<uint64_t>(static_cast<uint32_t>(rz) & 0x1FFFFF);
        const auto ud = static_cast<uint64_t>(dimensionType);

        return ud << 42 | ux << 21 | uz;
    }

    RegionLocation RegionLocation::fromRegionID(const RegionID regionID) {
        const auto ux = static_cast<int32_t>(regionID >> 21 & 0x1FFFFF);
        const auto uz = static_cast<int32_t>(regionID & 0x1FFFFF);
        const auto ud = static_cast<int32_t>(regionID >> 42 & 0xFFFFF);

        const auto recoveredX = ux >= 0x100000 ? ux - 0x200000 : ux;
        const auto recoveredZ = uz >= 0x100000 ? uz - 0x200000 : uz;
        const auto recoveredDim = static_cast<DimensionType>(ud);

        return RegionLocation{recoveredX, recoveredZ, recoveredDim};
    }

    Option<RegionLocation> RegionLocation::fromFileName(const DimensionType dimension, const fs::path& file) {
        const auto filename = file.filename().string();
        if (!filename.starts_with("r.")) return None;

        const auto extensionLength = filename.ends_with(".zvcr3") || filename.ends_with(".zvcr2") ? 6
                             : filename.ends_with(".zvr") || filename.ends_with(".zpr") ? 4
                             : 0;

        if (extensionLength == 0) return None;

        static constexpr auto startOffset = 2; // "r.", 2 chars
        const auto endOffset = extensionLength + startOffset;

        const auto regionIdentifier = filename.substr(startOffset, filename.length() - endOffset);
        const auto delimiter = regionIdentifier.find_first_of('.');
        if (delimiter == std::string::npos) return None;

        const auto regionX = std::stoi(regionIdentifier.substr(0, delimiter));
        const auto regionZ = std::stoi(regionIdentifier.substr(delimiter + 1));

        return RegionLocation{regionX, regionZ, dimension};
    }

    fs::path RegionLocation::getDirectory(const std::string& parentDirectory) const {
        const auto sectorX = std::to_string(rx / SECTOR_SIDELENGTH);
        const auto sectorZ = std::to_string(rz / SECTOR_SIDELENGTH);
        const auto dimID = std::to_string(static_cast<int32_t>(dimensionType));

        return fs::path(parentDirectory) / dimID / sectorX / sectorZ;
    }

    std::string RegionLocation::getFileName(const RegionFormat format) const {
        return ZVCR_REGION_PREFIX + std::to_string(rx)
             + ZVCR_REGION_DELIMITER + std::to_string(rz)
             + ZVCR_EXTENSION + std::to_string(static_cast<uint>(format));
    }

    fs::path RegionLocation::getFilePath(const fs::path& parentDirectory, const RegionFormat format) const {
        return fs::path(getDirectory(parentDirectory)) / getFileName(format);
    }

}