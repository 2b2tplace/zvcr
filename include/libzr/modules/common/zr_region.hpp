#pragma once

#include <libzr/modules/zr_common.hpp>

template<typename S>
class ZrRegion {
public:
    typedef std::optional<S> SegmentMaybe;
    typedef std::vector<std::optional<S>> Segments;

    explicit ZrRegion(const Segments& segments);
    ZrRegion() = default;

    SegmentMaybe get(uint8_t x, uint8_t z) const;
    void set(uint8_t x, uint8_t z, const SegmentMaybe& segment);
    static size_t unpackedIndex(uint8_t x, uint8_t z);

    Segments segments;
};