#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace zvcr::common::generic_region {

    template<typename S>
    class GenericRegion {
    public:
        using SegmentMaybe = std::optional<S>;
        using Segments = std::vector<std::optional<S>>;

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
