#include <zvcr/region/file_location.hpp>

namespace zvcr::region::file_location {

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
        const auto recoveredDim = static_cast<dimension::DimensionType>(ud);

        return RegionLocation {
            recoveredX, recoveredZ, recoveredDim
        };
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