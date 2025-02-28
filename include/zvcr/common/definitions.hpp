#pragma once

#include <cstddef>

// epoch.major.minor.patch.Pprotocol
// we only officially support 1.20.4-1.20.6!!!
// see https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge/Protocol_History
enum ZVCRCoreVersion {
    ZVCR_0_0_0_0_P765,
    ZVCR_0_0_1_0_P765
};

#if ZVCR_VERSION == ZVCR_0_0_0_0_P765 || ZVCR_VERSION == ZVCR_0_0_1_0_P765
#define PROTOCOL_VERSION 765
#endif

namespace zvcr::common::definitions {

    constexpr size_t REGION_SIDELENGTH_SEGMENTS = 32;
    constexpr size_t SEGMENTS_PER_REGION = REGION_SIDELENGTH_SEGMENTS * REGION_SIDELENGTH_SEGMENTS;

    constexpr size_t SEGMENT_SIDELENGTH_BLOCKS = 16;
    constexpr size_t SECTION_2D_SIZE_BLOCKS = SEGMENT_SIDELENGTH_BLOCKS * SEGMENT_SIDELENGTH_BLOCKS;
    constexpr size_t SECTION_3D_SIZE_BLOCKS = SEGMENT_SIDELENGTH_BLOCKS * SEGMENT_SIDELENGTH_BLOCKS * SEGMENT_SIDELENGTH_BLOCKS;

    using BlockStateId = uint16_t;
    using Segment2dAtom = uint16_t;
    using BiomeId = uint8_t;

}