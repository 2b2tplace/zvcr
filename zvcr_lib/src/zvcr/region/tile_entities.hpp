#pragma once

#include <zvcr/definitions.hpp>
#include <cstdint>
#include <ctime>
#include <ranges>
#include <unordered_map>
#include <vector>
#include <variant>

namespace zvcr {

    struct TileEntityPosition {
        uint8_t x{};
        uint8_t z{};
        uint16_t y{};

        [[nodiscard]]
        auto packedPosition() const -> uint32_t;

        [[nodiscard]]
        static auto unpack(uint32_t packedPosition) -> TileEntityPosition;

        auto operator==(const TileEntityPosition &other) const noexcept -> bool;
    };

    struct TileEntityPositionHash {
        auto operator()(const TileEntityPosition &pos) const noexcept -> size_t;
    };

    struct TileEntity {
        uint32_t type;
        TileEntityPosition pos;
        std::vector<uint8_t> nbt;

        auto operator==(const TileEntity &other) const noexcept -> bool;
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

        auto operator==(const TileEntityListDelta &other) const -> bool ;
    };

    struct DeltaTileEntityData {
        std::vector<TileEntityListDelta> reverseDeltas;

        [[nodiscard]]
        auto latestSnapshot() const -> result::OptionCRef<TileEntityListDelta>;

        [[nodiscard]]
        auto delta(size_t deltaIndex) const -> result::OptionCRef<TileEntityListDelta>;

        [[nodiscard]]
        auto snapshotBefore(time_t timestamp) const -> result::Option<TileEntityList>;

        [[nodiscard]]
        auto snapshotFrom(time_t timestamp) const -> result::Option<TileEntityList>;

        template<std::ranges::input_range TileEntitySource> requires std::ranges::sized_range<TileEntitySource>
        auto insertSnapshot(const time_t timestamp, const TileEntitySource &tileEntityListSnapshot) -> DeltaInsertionResult {
            const auto latestOpt = latestSnapshot();
            if (!latestOpt) {
                auto &[_, deltas] = reverseDeltas.emplace_back(timestamp);
                deltas.reserve(tileEntityListSnapshot.size());

                for (const auto &tileEntity : tileEntityListSnapshot)
                    deltas[tileEntity.pos] = tileEntity;

                return deltas.size();
            }
            const auto &latest = latestOpt->get();
            if (timestamp <= latest.timestamp) return ERR(DeltaInsertionStatus::SNAPSHOT_OLDER_THAN_LATEST);

            TileEntityListDelta newLatest{.timestamp = timestamp};
            newLatest.deltas.reserve(tileEntityListSnapshot.size());

            for (const auto &tileEntity : tileEntityListSnapshot)
                newLatest.deltas[tileEntity.pos] = tileEntity;

            TileEntityListDelta deltas{.timestamp = latest.timestamp};
            for (const auto &tileEntity : tileEntityListSnapshot) {
                if (const auto &found = latest.deltas.find(tileEntity.pos); found == latest.deltas.end()) {
                    deltas.deltas[tileEntity.pos] = std::monostate{};
                } else if (std::holds_alternative<TileEntity>(found->second) && std::get<TileEntity>(found->second) != tileEntity) {
                    deltas.deltas[tileEntity.pos] = found->second;
                }
            }
            for (const auto &[pos, delta] : latest.deltas) {
                if (!newLatest.deltas.contains(pos) && std::holds_alternative<TileEntity>(delta))
                    deltas.deltas[pos] = delta;
            }
            if (deltas.deltas.empty()) return ERR(DeltaInsertionStatus::NO_CHANGES_MADE);

            reverseDeltas.erase(reverseDeltas.begin());
            reverseDeltas.emplace(reverseDeltas.begin(), deltas);
            reverseDeltas.emplace(reverseDeltas.begin(), newLatest);
            return deltas.deltas.size();
        }
    };

}
