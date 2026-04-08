#pragma once

#include <cstdint>
#include <ctime>
#include <unordered_map>
#include <vector>
#include <zvcr/common/data_storage.hpp>

namespace zvcr {

    struct TileEntityPosition {
        uint8_t x{};
        uint8_t z{};
        int16_t y{};

        [[nodiscard]]
        auto packedPosition() const -> uint32_t;

        [[nodiscard]]
        static auto unpack(uint32_t packedPosition) -> TileEntityPosition ;

        auto operator==(const TileEntityPosition &other) const noexcept -> bool;
    };

    struct TileEntityPositionHash {
        auto operator()(const TileEntityPosition &pos) const noexcept -> size_t;
    };

    struct TileEntity {
        uint32_t type;
        TileEntityPosition pos;
        std::vector<uint8_t> nbt;

        auto operator==(const TileEntity &other) const noexcept -> bool ;
    };

    using TileEntityDelta = std::variant<
        TileEntity, // put
        std::monostate // erase
    >;

    using TileEntityDeltaMap = std::unordered_map<TileEntityPosition, TileEntityDelta, TileEntityPositionHash>;
    using TileEntityList = std::unordered_map<TileEntityPosition, TileEntity, TileEntityPositionHash>;

    struct TileEntityListDelta {
        time_t timestamp;
        TileEntityDeltaMap deltas{};
    };

    struct DeltaTileEntityData {
        std::vector<TileEntityListDelta> reverseDeltas;

        [[nodiscard]]
        auto latestSnapshot() const -> result::OptionCRef<TileEntityListDelta>;

        [[nodiscard]]
        auto delta(size_t deltaIndex) const -> result::OptionCRef<TileEntityListDelta>;

        auto insertSnapshot(time_t timestamp, const std::vector<TileEntity> &tileEntityListSnapshot) -> DeltaInsertionResult;

        [[nodiscard]]
        auto snapshotFrom(time_t timestamp) const -> result::Option<TileEntityList>;
    };

}
