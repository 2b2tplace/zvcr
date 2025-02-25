#pragma once

#include <cstdint>
#include <vector>

#include "result.hpp"

namespace zvcr::common::generic_region {

    template<typename S>
    class GenericRegion {
    public:
        using SegmentMaybe = result::Option<S>;
        using Segments = std::vector<SegmentMaybe>;

        Segments segments;

        explicit GenericRegion(const Segments& segments);
        GenericRegion() = default;

        [[nodiscard]]
        SegmentMaybe get(uint8_t x, uint8_t z) const;

        void set(uint8_t x, uint8_t z, const SegmentMaybe& segment);

        [[nodiscard]]
        static size_t unpackedIndex(uint8_t x, uint8_t z);

    };

}
