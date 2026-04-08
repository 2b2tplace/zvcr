#pragma once

#include <cassert>
#include <memory>
#include <zvcr/common/definitions.hpp>

namespace zvcr {

    template<typename S>
    class GenericRegion {
    public:
        using SegmentMaybe = std::shared_ptr<S>;
        using Segments = std::array<std::shared_ptr<S>, SEGMENTS_PER_REGION>;

        Segments segments;
        uint16_t protocolVersion;

        explicit GenericRegion(Segments&& segments, const uint16_t protocolVersion) noexcept:
            segments(std::move(segments)), protocolVersion(protocolVersion) {}

        explicit GenericRegion(const Segments& segments, const uint16_t protocolVersion):
            segments(segments), protocolVersion(protocolVersion) {}

        explicit GenericRegion(const uint16_t protocolVersion):
            segments(), protocolVersion(protocolVersion) {}

        GenericRegion(GenericRegion &&other) noexcept:
            segments(std::move(other.segments)),
            protocolVersion(other.protocolVersion) {}

        GenericRegion(const GenericRegion &other) = default;

        GenericRegion(): GenericRegion(0) {}

        [[nodiscard]]
        auto get(const uint8_t x, const uint8_t z) const -> SegmentMaybe {
            return segments[unpackedIndex(x, z)];
        }

        auto set(const uint8_t x, const uint8_t z, const SegmentMaybe& segment) -> void {
            segments[unpackedIndex(x, z)] = segment;
        }

        [[nodiscard]]
        static auto unpackedIndex(const uint8_t x, const uint8_t z) -> size_t {
            assert(x < REGION_SIDELENGTH_SEGMENTS);
            assert(z < REGION_SIDELENGTH_SEGMENTS);
            return static_cast<size_t>(x) * REGION_SIDELENGTH_SEGMENTS + static_cast<size_t>(z);
        }

    };

}
