#pragma once

#include <vector>
#include <cstdint>

namespace zvcr::region {

    using TileEntityType = uint16_t;

    struct TileEntityCountInfo {
        std::vector<TileEntityType> counts{};
        time_t timestamp{};
    };

    using TileEntityCounts = std::vector<TileEntityCountInfo>;

    [[nodiscard]]
    inline size_t getTotalTileEntities(const uint16_t protocolVersion) {
        if (protocolVersion >= 768) return 45;
        if (protocolVersion >= 766) return 44;
        if (protocolVersion >= 765) return 41;

        return -1; // versions pre 1.20.4 are not supported
    }

}