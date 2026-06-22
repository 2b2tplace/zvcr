#pragma once

#include <result.hpp>
#include <cstddef>
#include <cstdint>

namespace zvcr {

    inline constexpr size_t REGION_SIDELENGTH_SEGMENTS = 32;
    inline constexpr size_t SEGMENTS_PER_REGION = REGION_SIDELENGTH_SEGMENTS * REGION_SIDELENGTH_SEGMENTS;

    inline constexpr size_t SEGMENT_SIDELENGTH_BLOCKS = 16;
    inline constexpr size_t SECTION_SIZE_BLOCKS = SEGMENT_SIDELENGTH_BLOCKS * SEGMENT_SIDELENGTH_BLOCKS * SEGMENT_SIDELENGTH_BLOCKS;

    inline constexpr size_t SEGMENT_SIDELENGTH_BIOMES = 4;
    inline constexpr size_t SECTION_SIZE_BIOMES = SEGMENT_SIDELENGTH_BIOMES * SEGMENT_SIDELENGTH_BIOMES * SEGMENT_SIDELENGTH_BIOMES;

    using SegmentAtom = uint16_t;

    inline constexpr SegmentAtom STATE_UNCHANGED = 0xFFFF;

    enum class DeltaInsertionStatus {
        SNAPSHOT_OLDER_THAN_LATEST,
        NO_CHANGES_MADE
    };

    using DeltaInsertionResult = result::Result<size_t, DeltaInsertionStatus>;

}