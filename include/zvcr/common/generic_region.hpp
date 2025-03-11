#pragma once

#include <cstdint>
#include <vector>
#include <cassert>
#include <zvcr/common/result.hpp>
#include <zvcr/common/definitions.hpp>

namespace zvcr::common::generic_region {

    using definitions::REGION_SIDELENGTH_SEGMENTS;
    using definitions::SEGMENTS_PER_REGION;

    template<typename S>
    class GenericRegion {
    public:
        using SegmentMaybe = result::Option<S>;
        using Segments = std::vector<SegmentMaybe>;

        Segments segments;

        explicit GenericRegion(Segments&& segments) noexcept: segments(std::move(segments)) {}
        explicit GenericRegion(const Segments& segments): segments(segments) {}
        GenericRegion(): segments(SEGMENTS_PER_REGION) {}

        [[nodiscard]]
        SegmentMaybe get(const uint8_t x, const uint8_t z) const {
            return segments[unpackedIndex(x, z)];
        }

        void set(const uint8_t x, const uint8_t z, const SegmentMaybe& segment) {
            segments[unpackedIndex(x, z)] = segment;
        }

        [[nodiscard]]
        static size_t unpackedIndex(const uint8_t x, const uint8_t z) {
            assert(x < REGION_SIDELENGTH_SEGMENTS);
            assert(z < REGION_SIDELENGTH_SEGMENTS);
            return static_cast<size_t>(x) * REGION_SIDELENGTH_SEGMENTS + static_cast<size_t>(z);
        }

    };

}
