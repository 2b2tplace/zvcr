#include <zvcr/common/generic_region.hpp>
#include <zvcr/common/definitions.hpp>
#include <zvcr/region/dim3/segment3.hpp>
#include <zvcr/region/dim2/segment2.hpp>

#include <cassert>
#include <cstdint>

namespace zvcr::common::generic_region {

    using definitions::REGION_SIDELENGTH_SEGMENTS;

    template<typename S>
    typename GenericRegion<S>::SegmentMaybe GenericRegion<S>::get(const uint8_t x, const uint8_t z) const {
        return segments[unpackedIndex(x, z)];
    }

    template<typename S>
    void GenericRegion<S>::set(const uint8_t x, const uint8_t z, const SegmentMaybe& segment) {
        segments[unpackedIndex(x, z)] = segment;
    }

    template<typename S>
    size_t GenericRegion<S>::unpackedIndex(const uint8_t x, const uint8_t z) {
        assert(x < REGION_SIDELENGTH_SEGMENTS);
        assert(z < REGION_SIDELENGTH_SEGMENTS);
        return static_cast<size_t>(x) * REGION_SIDELENGTH_SEGMENTS + static_cast<size_t>(z);
    }

    template<typename S>
    GenericRegion<S>::GenericRegion(const Segments& segments) {
        this->segments = segments;
    }

    template class GenericRegion<region::dim3::segment3::Segment3d>;
    template class GenericRegion<region::dim2::segment2::Segment2d>;

}
