#include <zvcr/region/segment/tile_entities.hpp>

namespace zvcr {

    auto TileEntityPosition::packedPosition() const -> uint32_t {
        return static_cast<uint32_t>(static_cast<uint16_t>(y)) << 16
            | static_cast<uint32_t>(z) << 8
            | static_cast<uint32_t>(x);
    }

    auto TileEntityPosition::unpack(const uint32_t packedPosition) -> TileEntityPosition {
        return TileEntityPosition {
            .x = static_cast<uint8_t>(packedPosition & 0xFF),
            .z = static_cast<uint8_t>(packedPosition >> 8 & 0xFF),
            .y = static_cast<int16_t>(packedPosition >> 16)
        };
    }

    auto TileEntityPosition::operator==(const TileEntityPosition &other) const noexcept -> bool {
        return x == other.x && y == other.y && z == other.z;
    }

    auto TileEntityPositionHash::operator()(const TileEntityPosition &pos) const noexcept -> size_t {
        return pos.packedPosition();
    }

    auto TileEntity::operator==(const TileEntity &other) const noexcept -> bool {
        return type == other.type && pos == other.pos && nbt == other.nbt;
    }

    auto DeltaTileEntityData::latestSnapshot() const -> result::OptionCRef<TileEntityListDelta> {
        return delta(0);
    }

    auto DeltaTileEntityData::delta(const size_t deltaIndex) const -> result::OptionCRef<TileEntityListDelta> {
        return deltaIndex >= reverseDeltas.size() ? result::None : result::OptionCRef<TileEntityListDelta>{reverseDeltas[deltaIndex]};
    }

    auto DeltaTileEntityData::insertSnapshot(const time_t timestamp, const std::vector<TileEntity> &tileEntityListSnapshot) -> DeltaInsertionResult {
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
        reverseDeltas.erase(reverseDeltas.begin());
        reverseDeltas.emplace(reverseDeltas.begin(), deltas);
        reverseDeltas.emplace(reverseDeltas.begin(), newLatest);
        return deltas.deltas.size();
    }

    auto DeltaTileEntityData::snapshotFrom(const time_t timestamp) const -> result::Option<TileEntityList> {
        const auto latestOpt = latestSnapshot();
        if (!latestOpt) return result::None;

        const auto &latest = latestOpt->get();
        TileEntityList snapshot;
        snapshot.reserve(latest.deltas.size());
        for (const auto &[pos, delta] : latest.deltas) {
            if (std::holds_alternative<TileEntity>(delta))
                snapshot[pos] = std::get<TileEntity>(delta);
        }
        if (timestamp >= latest.timestamp) return snapshot;

        bool first = true;
        for (const auto &[deltaTimestamp, deltas] : reverseDeltas) {
            if (first) {
                first = false;
                continue;
            }
            for (const auto &[pos, delta] : deltas) {
                if (std::holds_alternative<TileEntity>(delta)) {
                    snapshot[pos] = std::get<TileEntity>(delta);
                } else {
                    snapshot.erase(pos);
                }
            }
            if (timestamp >= deltaTimestamp) break;
        }
        return snapshot;
    }
}
