#pragma once

#include <vector>
#include <result.hpp>
#include <zvcr/region/segment/tile_entities.hpp>

namespace zvcr {

    enum class SegmentStateType {
        UNKNOWN = 0,
        NEW = 1,
        OLD = 2
    };

    struct SegmentState {
        SegmentStateType type;
        time_t timestamp;
    };

    using SegmentStates = std::vector<SegmentState>;

    class SegmentInfo {
    public:
        SegmentStates segmentStates{};
        TileEntityCounts tileEntityCounts{};

        explicit SegmentInfo(const SegmentState& initialState, const TileEntityCountInfo& initialTileEntityCounts);

        explicit SegmentInfo(const SegmentStates& chunkStates, const TileEntityCounts& tileEntities);

        SegmentInfo() = default;

        [[nodiscard]]
        result::OptionCRef<SegmentState> latestState() const;

        [[nodiscard]]
        result::OptionCRef<TileEntityCountInfo> latestTileEntityCounts() const;

        [[nodiscard]]
        result::Option<SegmentState> stateFrom(time_t timestamp) const;

        [[nodiscard]]
        result::Option<TileEntityCountInfo> tileEntityCountsFrom(time_t timestamp) const;

        [[nodiscard]]
        bool updateState(const SegmentState& newState);

        [[nodiscard]]
        bool updateTileEntityCounts(const TileEntityCountInfo& newTileEntityCounts);
    };

}