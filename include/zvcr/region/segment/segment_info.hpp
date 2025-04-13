#pragma once

#include <zvcr/region/segment/tile_entities.hpp>

#include <ctime>
#include <map>
#include <string>
#include <vector>
#include <zvcr/common/result.hpp>

namespace zvcr::region {

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

    using namespace result;

    class SegmentInfo {
    public:
        SegmentStates segmentStates{};
        TileEntityCounts tileEntityCounts{};

        explicit SegmentInfo(const SegmentState& initialState, const TileEntityCountInfo& initialTileEntityCounts);

        explicit SegmentInfo(const SegmentStates& chunkStates, const TileEntityCounts& tileEntities);

        SegmentInfo() = default;

        [[nodiscard]]
        OptionCRef<SegmentState> latestState() const;

        [[nodiscard]]
        OptionCRef<TileEntityCountInfo> latestTileEntityCounts() const;

        [[nodiscard]]
        Option<SegmentState> stateFrom(time_t timestamp) const;

        [[nodiscard]]
        Option<TileEntityCountInfo> tileEntityCountsFrom(time_t timestamp) const;

        [[nodiscard]]
        bool updateState(const SegmentState& newState);

        [[nodiscard]]
        bool updateTileEntityCounts(const TileEntityCountInfo& newTileEntityCounts);
    };

}
