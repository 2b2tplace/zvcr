#include <libzr/modules/common/zr_region.hpp>
#include <libzr/modules/zvr/zvr_chunk.hpp>
#include <libzr/modules/zpr/zpr_segment.hpp>

template<typename S>
typename ZrRegion<S>::SegmentMaybe ZrRegion<S>::get(const uint8_t x, const uint8_t z) const {
    return segments[unpackedIndex(x, z)];
}

template<typename S>
void ZrRegion<S>::set(const uint8_t x, const uint8_t z, const SegmentMaybe& segment) {
    segments[unpackedIndex(x, z)] = segment;
}

template<typename S>
size_t ZrRegion<S>::unpackedIndex(const uint8_t x, const uint8_t z) {
    assert(x < REGION_SIDELENGTH);
    assert(z < REGION_SIDELENGTH);
    return static_cast<size_t>(x) * REGION_SIDELENGTH + static_cast<size_t>(z);
}

template<typename S>
ZrRegion<S>::ZrRegion(const Segments& segments) {
    this->segments = segments;
}

template class ZrRegion<ZvrChunk>;
template class ZrRegion<ZprSegment>;