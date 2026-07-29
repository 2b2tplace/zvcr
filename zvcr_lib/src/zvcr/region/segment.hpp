#pragma once

#include <zvcr/region/tile_entities.hpp>
#include <zvcr/region/paletted_delta_data.hpp>
#include <zvcr/region/segment_info.hpp>
#include <zvcr/dimension.hpp>

namespace zvcr {

    inline constexpr size_t MAX_SECTION_COUNT = 24;

    template<size_t unpackedSize>
    class DeltaSections {
        using SegmentSnapshot = std::vector<UnpackedData<unpackedSize>>;
        using SegmentPackedSnapshot = std::vector<PackedSnapshot<unpackedSize>>;
        using PackedData = PackedDeltaData<unpackedSize>;

    public:
        std::array<PackedData, MAX_SECTION_COUNT> sections{};
        size_t sectionCount;

        explicit DeltaSections(const size_t sectionCount): sectionCount(sectionCount) {}

        [[nodiscard]]
        auto snapshotFrom(const time_t timestamp) const -> result::Option<SegmentSnapshot> {
            SegmentSnapshot snapshots;
            snapshots.reserve(sectionCount);

            for (size_t i = 0; i < sectionCount; ++i) {
                const auto &snapshot = sections[i].snapshotFrom(timestamp);
                if (!snapshot) return result::None;

                snapshots.push_back(*snapshot);
            }
            return snapshots;
        }

        [[nodiscard]]
        auto latestSnapshot(time_t *getEarliestTimestamp = nullptr) const -> result::Option<SegmentSnapshot> {
            SegmentSnapshot snapshots;
            snapshots.reserve(sectionCount);

            for (size_t i = 0; i < sectionCount; ++i) {
                const auto &snapshot = sections[i].latestSnapshot();
                if (!snapshot) return result::None;

                if (const auto timestamp = snapshot->get().timestamp; getEarliestTimestamp && timestamp < *getEarliestTimestamp)
                    *getEarliestTimestamp = timestamp;

                snapshots.push_back(snapshot->get().data.unpack());
            }
            return snapshots;
        }

        [[nodiscard]]
        auto updateSections(const SegmentPackedSnapshot &sectionUpdates) -> size_t {
            size_t changes = 0;
            for (size_t section = 0; section < sectionUpdates.size() && section < sectionCount; ++section)
                changes += sections[section].insertSnapshot(sectionUpdates[section]).value_or(0);

            return changes;
        }
    };

    struct Segment {
        size_t sectionCount;
        DeltaSections<SECTION_SIZE_BLOCKS> blockSections;
        DeltaSections<SECTION_SIZE_BIOMES> biomeSections;
        SegmentInfo info;
        DeltaTileEntityData tileEntities;

        explicit Segment(const DimensionType dimension):
            Segment(dimensionSectionCount(dimension)) {}

        explicit Segment(const size_t sectionCount):
            sectionCount(sectionCount),
            blockSections(sectionCount),
            biomeSections(sectionCount),
            info(SegmentStates{}),
            tileEntities({}) {}
    };

    class Region {
    public:
        using SegmentMaybe = std::shared_ptr<Segment>;
        using Segments = std::array<std::shared_ptr<Segment>, SEGMENTS_PER_REGION>;

        Segments segments;
        uint16_t protocolVersion;

        explicit Region(Segments &&segments, const uint16_t protocolVersion) noexcept:
            segments(std::move(segments)), protocolVersion(protocolVersion) {}

        explicit Region(const Segments &segments, const uint16_t protocolVersion):
            segments(segments), protocolVersion(protocolVersion) {}

        explicit Region(const uint16_t protocolVersion): protocolVersion(protocolVersion) {}

        Region(Region &&other) noexcept:
            segments(std::move(other.segments)),
            protocolVersion(other.protocolVersion) {}

        Region(const Region &other) = default;

        [[nodiscard]]
        auto get(const uint8_t x, const uint8_t z) const -> SegmentMaybe {
            return segments[segmentIndex(x, z)];
        }

        auto set(const uint8_t x, const uint8_t z, const SegmentMaybe &segment) -> void {
            segments[segmentIndex(x, z)] = segment;
        }

        [[nodiscard]]
        static auto segmentIndex(const uint8_t x, const uint8_t z) -> size_t {
            assert(x < REGION_SIDELENGTH_SEGMENTS && z < REGION_SIDELENGTH_SEGMENTS);
            return static_cast<size_t>(x) * REGION_SIDELENGTH_SEGMENTS + static_cast<size_t>(z);
        }
    };

}
