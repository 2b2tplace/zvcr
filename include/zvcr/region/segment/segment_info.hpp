#pragma once

#include <zvcr/region/segment/tile_entities.hpp>

#include <ctime>
#include <map>
#include <string>
#include <vector>
#include <optional>

namespace zvcr::region::segment::segment_info {

    enum class SegmentStateType {
        UNKNOWN = 0,
        NEW = 1,
        OLD = 2
    };

    static const std::map<SegmentStateType, std::string> SegmentStateTypeToString = {
        {SegmentStateType::UNKNOWN, "unknown"},
        {SegmentStateType::NEW, "new"},
        {SegmentStateType::OLD, "old"},
    };

    [[nodiscard]]
    inline std::string toString(const SegmentStateType state) {
        return SegmentStateTypeToString.at(state);
    }

    struct SegmentState {
        SegmentStateType type{};
        time_t timestamp{};
    };

    using SegmentStates = std::vector<SegmentState>;

    using tile_entities::TileEntityCounts;
    using tile_entities::TileEntityCountInfo;

    class SegmentInfo {
    public:
        SegmentStates segmentStates{};
        TileEntityCounts tileEntityCounts{};

        explicit SegmentInfo(const SegmentState& initialState, const TileEntityCountInfo& initialTileEntityCounts);

        explicit SegmentInfo(const SegmentStates& chunkStates, const TileEntityCounts& tileEntities);

        SegmentInfo() = default;

        [[nodiscard]]
        std::optional<SegmentState> latestState() const;

        [[nodiscard]]
        std::optional<TileEntityCountInfo> latestTileEntityCounts() const;

        [[nodiscard]]
        std::optional<SegmentState> stateFrom(time_t timestamp) const;

        [[nodiscard]]
        std::optional<TileEntityCountInfo> tileEntityCountsFrom(time_t timestamp) const;

        [[nodiscard]]
        bool updateState(const SegmentState& newState);

        [[nodiscard]]
        bool updateTileEntityCounts(const TileEntityCountInfo& newTileEntityCounts);
    };

}