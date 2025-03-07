#pragma once

#include <cstddef>
#include <cstdint>

namespace zvcr::common::definitions {

    constexpr uint8_t REGION_SIDELENGTH_SEGMENTS = 32;
    constexpr size_t SEGMENTS_PER_REGION = static_cast<size_t>(REGION_SIDELENGTH_SEGMENTS) * static_cast<size_t>(REGION_SIDELENGTH_SEGMENTS);

    constexpr uint8_t SEGMENT_SIDELENGTH_BLOCKS = 16;
    constexpr size_t SECTION_2D_SIZE_BLOCKS = static_cast<size_t>(SEGMENT_SIDELENGTH_BLOCKS) * static_cast<size_t>(SEGMENT_SIDELENGTH_BLOCKS);
    constexpr size_t SECTION_3D_SIZE_BLOCKS = SECTION_2D_SIZE_BLOCKS * static_cast<size_t>(SEGMENT_SIDELENGTH_BLOCKS);

    constexpr uint8_t SEGMENT_SIDELENGTH_BIOMES = 4;
    constexpr size_t SECTION_2D_SIZE_BIOMES = static_cast<size_t>(SEGMENT_SIDELENGTH_BIOMES) * static_cast<size_t>(SEGMENT_SIDELENGTH_BIOMES);
    constexpr size_t SECTION_3D_SIZE_BIOMES = SECTION_2D_SIZE_BIOMES * static_cast<size_t>(SEGMENT_SIDELENGTH_BIOMES);

    using SegmentAtom = uint16_t;

}