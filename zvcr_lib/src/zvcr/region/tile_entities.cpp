#include <zvcr/region/tile_entities.hpp>
#include <zvcr/time_utils.hpp>

namespace zvcr {

    auto TileEntityPosition::packedPosition() const -> uint32_t {
        return static_cast<uint32_t>(y) << 16
            | static_cast<uint32_t>(z) << 8
            | static_cast<uint32_t>(x);
    }

    auto TileEntityPosition::unpack(const uint32_t packedPosition) -> TileEntityPosition {
        return TileEntityPosition {
            .x = static_cast<uint8_t>(packedPosition & 0xFF),
            .z = static_cast<uint8_t>(packedPosition >> 8 & 0xFF),
            .y = static_cast<uint16_t>(packedPosition >> 16)
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

    auto TileEntityListDelta::operator==(const TileEntityListDelta &other) const -> bool {
        return timestamp == other.timestamp && deltas == other.deltas;
    }

    auto DeltaTileEntityData::latestSnapshot() const -> result::OptionCRef<TileEntityListDelta> {
        return delta(0);
    }

    auto DeltaTileEntityData::delta(const size_t deltaIndex) const -> result::OptionCRef<TileEntityListDelta> {
        return deltaIndex >= reverseDeltas.size() ? result::None : result::OptionCRef<TileEntityListDelta>{reverseDeltas[deltaIndex]};
    }

    auto DeltaTileEntityData::snapshotBefore(const time_t timestamp) const -> result::Option<TileEntityList> {
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

    auto DeltaTileEntityData::snapshotFrom(const time_t timestamp) const -> result::Option<TileEntityList> {
        return snapshotBefore(findNearestTimestamp(reverseDeltas, timestamp));
    }
}
