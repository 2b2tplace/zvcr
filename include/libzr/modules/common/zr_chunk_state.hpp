#pragma once

#include <libzr/modules/zr_common.hpp>

enum class ZrChunkStateType {
    UNKNOWN = 0,
    NEW = 1,
    OLD = 2
};

static const std::map<ZrChunkStateType, std::string> ChunkStateTypeRegistry = {
    {ZrChunkStateType::UNKNOWN, "unknown"},
    {ZrChunkStateType::NEW, "new"},
    {ZrChunkStateType::OLD, "old"},
};

struct ZrChunkState {
    ZrChunkStateType type{};
    time_t timestamp{};
};

typedef std::vector<ZrChunkState> ChunkStates;

std::ostream& operator<<(std::ostream& os, ZrChunkStateType chunkStateType);