#pragma once

#include <zvcr/definitions.hpp>
#include <zvcr/time_utils.hpp>
#include <absl/hash/hash.h>
#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <tuple>
#include <variant>

namespace zvcr {

    using LongArray = std::vector<uint64_t>;

    template<size_t>
    class PackedData;

    template<size_t unpackedSize>
    using UnpackedData = std::array<SegmentAtom, unpackedSize>;

    template<size_t>
    struct PackedSnapshot;

    template<size_t unpackedSize>
    class UnpackedView {
    public:
        UnpackedData<unpackedSize> unpacked;

        explicit UnpackedView(const uint8_t sidelength): sidelength_(sidelength) {
            this->unpacked = UnpackedData<unpackedSize>{};
        }

        explicit UnpackedView(const uint8_t sidelength, const SegmentAtom fill): sidelength_(sidelength) {
            this->unpacked = UnpackedData<unpackedSize>{};
            this->unpacked.fill(fill);
        }

        explicit UnpackedView(const uint8_t sidelength, const UnpackedData<unpackedSize> &unpacked): sidelength_(sidelength) {
            this->unpacked = unpacked;
        }

        [[nodiscard]]
        auto voxel(const uint8_t x, const uint8_t y, const uint8_t z) const -> SegmentAtom {
            return unpacked[unpackedIndex(x, y, z)];
        }

        auto voxel(const uint8_t x, const uint8_t y, const uint8_t z, const SegmentAtom voxel) -> void {
            unpacked[unpackedIndex(x, y, z)] = voxel;
        }

        [[nodiscard]]
        auto pixel(const uint8_t x, const uint8_t z) const -> SegmentAtom {
            return unpacked[unpackedIndex(x, z)];
        }

        auto pixel(const uint8_t x, const uint8_t z, const SegmentAtom pixel) -> void {
            unpacked[unpackedIndex(x, z)] = pixel;
        }

        [[nodiscard]]
        auto pack() const -> PackedData<unpackedSize> {
            return PackedData<unpackedSize>::pack(unpacked);
        }

        [[nodiscard]]
        auto packSnapshot(time_t timestamp) const -> PackedSnapshot<unpackedSize> {
            return PackedSnapshot{pack(), timestamp};
        }

        [[nodiscard]]
        auto unpackedIndex(const uint8_t x, const uint8_t y, const uint8_t z) const -> size_t {
            assert(x < sidelength && "X coordinate out of bounds");
            assert(y < sidelength && "Y coordinate out of bounds");
            assert(z < sidelength && "Z coordinate out of bounds");

            return static_cast<size_t>(y) * sidelength_ * sidelength_
                 + static_cast<size_t>(z) * sidelength_
                 + static_cast<size_t>(x);
        }

        [[nodiscard]]
        auto sidelength() const -> uint8_t {
            return sidelength_;
        }

        [[nodiscard]]
        auto unpackedIndex(const uint8_t x, const uint8_t z) const -> size_t {
            return unpackedIndex(x, 0, z);
        }
    private:
        uint8_t sidelength_;
    };

    [[nodiscard]]
    inline auto createBlockView(const SegmentAtom fill = 0) -> UnpackedView<SECTION_SIZE_BLOCKS> {
        return UnpackedView<SECTION_SIZE_BLOCKS>{SEGMENT_SIDELENGTH_BLOCKS, fill};
    }

    [[nodiscard]]
    inline auto createBiomeView(const SegmentAtom fill = 0) -> UnpackedView<SECTION_SIZE_BIOMES> {
        return UnpackedView<SECTION_SIZE_BIOMES>{SEGMENT_SIDELENGTH_BIOMES, fill};
    }

    inline constexpr size_t MAX_INDIRECT_PALETTE_SIZE = UINT8_MAX + 1;

    [[nodiscard]]
    inline auto bitsPerEntry(const size_t paletteLength) -> size_t {
        if (paletteLength <= 16) return 4;
        if (paletteLength <= MAX_INDIRECT_PALETTE_SIZE) return 8;

        return 16;
    }

    using VectorPalette = std::vector<SegmentAtom>;

    struct Palette {
        VectorPalette palette{};
        size_t bitsPerEntry{};

        [[nodiscard]]
        auto equals(const Palette &other) const -> bool {
            return bitsPerEntry == other.bitsPerEntry && palette == other.palette;
        }

        [[nodiscard]]
        auto operator==(const Palette &other) const -> bool {
            return equals(other);
        }

        [[nodiscard]]
        auto length() const -> size_t {
            return palette.size();
        }

        [[nodiscard]]
        auto direct() const -> bool {
            return palette.empty();
        }
    };

    struct PaletteHash {
        size_t operator()(const Palette &p) const {
            size_t seed = p.bitsPerEntry;

            for (const auto atom : p.palette)
                seed ^= std::hash<SegmentAtom>{}(atom) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);

            return seed;
        }
    };

    static const auto DIRECT_PALETTE = Palette{.bitsPerEntry = 16};

    template<size_t unpackedSize>
    [[nodiscard]]
    auto buildPalette(const UnpackedData<unpackedSize> &data, std::array<uint8_t, UINT16_MAX + 1> &indices) -> Palette {
        Palette palette;
        std::bitset<UINT16_MAX + 1> unique;
        for (const auto atom : data) {
            if (unique.test(atom)) continue;
            unique.set(atom);

            if (palette.length() > MAX_INDIRECT_PALETTE_SIZE)
                return DIRECT_PALETTE;

            indices[atom] = static_cast<uint8_t>(palette.length());
            palette.palette.push_back(atom);
        }
        palette.bitsPerEntry = bitsPerEntry(palette.length());
        return palette;
    }

    template<size_t unpackedSize>
    struct PalettedData {
        explicit PalettedData(const Palette &palette) {
            const auto valuesPerLong = 64 / palette.bitsPerEntry;
            this->packedLongArray.resize((unpackedSize + valuesPerLong - 1) / valuesPerLong);
            this->palette = palette;
        }

        explicit PalettedData(const Palette &palette, const LongArray &packedLongArray):
            PalettedData(palette) {
            this->packedLongArray = packedLongArray;
        }

        PalettedData() = default;

        LongArray packedLongArray;
        Palette palette;
    };

    template<size_t unpackedSize>
    using Data = std::variant<PalettedData<unpackedSize>, SegmentAtom>;

    template<size_t unpackedSize>
    class PackedData {
    public:
        explicit PackedData(const Palette &palette):
            data(PalettedData<unpackedSize>{palette}) {}

        explicit PackedData(const Palette &palette, const LongArray &packedData):
            data(PalettedData<unpackedSize>{palette, packedData}) {}

        explicit PackedData(const PalettedData<unpackedSize> &palettedData):
            data(palettedData) {}

        explicit PackedData(const uint16_t singleValue):
            data(singleValue) {}

        PackedData() = default;

        [[nodiscard]]
        static auto pack(const UnpackedData<unpackedSize> &sectionData) -> PackedData {
            std::array<uint8_t, UINT16_MAX + 1> indices{};
            const auto palette = buildPalette(sectionData, indices);
            if (palette.length() == 1)
                return PackedData{palette.palette[0]};

            auto packedData = PackedData{palette};
            auto &palettedData = std::get<PalettedData<unpackedSize>>(packedData.data);
            auto &packedLongArray = palettedData.packedLongArray;

            const auto bits = static_cast<uint8_t>(palette.bitsPerEntry);
            const auto mask = (static_cast<uint64_t>(1) << bits) - 1;

            size_t unpackedIndex = 0;
            for (size_t cellIndex = 0; cellIndex < packedLongArray.size(); cellIndex++) {
                auto cell = 0UL;
                for (uint8_t bitIndex = 0; bitIndex < 64; bitIndex += bits) {
                    assert(unpackedIndex < unpackedSize && "Unpacked index out of bounds");
                    auto slice = sectionData.at(unpackedIndex);
                    if (!palette.direct()) {
                        assert(slice < indices.size() && "Palette index out of bounds");
                        slice = indices.at(slice);
                    }
                    cell = cell & ~(mask << bitIndex) | (slice & mask) << bitIndex;
                    ++unpackedIndex;
                }
                packedLongArray[cellIndex] = cell;
            }
            return packedData;
        }

        [[nodiscard]]
        auto unpack() const -> UnpackedData<unpackedSize> {
            if (std::holds_alternative<SegmentAtom>(data)) {
                auto result = UnpackedData<unpackedSize>{};
                result.fill(std::get<SegmentAtom>(data));
                return result;
            }
            UnpackedData<unpackedSize> unpacked{};
            const auto &palettedData = std::get<PalettedData<unpackedSize>>(data);
            const auto &palette = palettedData.palette;

            const auto &packedLongArray = palettedData.packedLongArray;
            const auto bits = static_cast<uint8_t>(palette.bitsPerEntry);
            const auto mask = (static_cast<uint64_t>(1) << bits) - 1;

            size_t unpackedIndex = 0;
            for (size_t cellIndex = 0; cellIndex < packedLongArray.size(); cellIndex++) {
                const auto cell = packedLongArray[cellIndex];
                for (uint8_t bitIndex = 0; bitIndex < 64; bitIndex += bits) {
                    auto slice = cell >> bitIndex & mask;
                    if (!palette.direct()) {
                        assert(slice < palette.length() && "Palette slice out of bounds");
                        slice = palette.palette.at(slice);
                    }
                    assert(unpackedIndex < unpackedSize && "Unpacked index out of bounds");
                    unpacked.at(unpackedIndex++) = slice;
                }
            }
            return unpacked;
        }

        [[nodiscard]]
        auto view(const uint8_t sidelength) const -> UnpackedView<unpackedSize> {
            return UnpackedView{sidelength, unpack()};
        }

        Data<unpackedSize> data;
    };

    template<size_t unpackedSize>
    struct PackedSnapshot {
        PackedData<unpackedSize> data;
        time_t timestamp{};
    };

    template<size_t unpackedSize>
    using PackedSnapshotVector = std::vector<PackedSnapshot<unpackedSize>>;

    template<size_t unpackedSize>
    class PackedDeltaData {
    public:
        PackedSnapshotVector<unpackedSize> reverseDeltas;

        explicit PackedDeltaData(const PackedSnapshot<unpackedSize> &initialState) {
            this->reverseDeltas.push_back(initialState);
        }

        explicit PackedDeltaData(PackedSnapshotVector<unpackedSize> reverseDeltas):
            reverseDeltas(std::move(reverseDeltas)) {}

        PackedDeltaData(): reverseDeltas() {}

        [[nodiscard]]
        auto latestSnapshot() const -> result::OptionCRef<PackedSnapshot<unpackedSize>> {
            return delta(0);
        }

        [[nodiscard]]
        auto delta(const size_t deltaIndex) const -> result::OptionCRef<PackedSnapshot<unpackedSize>> {
            return deltaIndex >= reverseDeltas.size() ? result::None : result::OptionCRef<PackedSnapshot<unpackedSize>>{reverseDeltas[deltaIndex]};
        }

        [[nodiscard]]
        auto snapshotFrom(const time_t timestamp) const -> result::Option<UnpackedData<unpackedSize>> {
            return snapshotBefore(findNearestTimestamp(reverseDeltas, timestamp));
        }

        [[nodiscard]]
        auto snapshotBefore(const time_t timestamp) const -> result::Option<UnpackedData<unpackedSize>> {
            const auto &latestPackedOpt = this->latestSnapshot();
            if (!latestPackedOpt) return result::None;

            const auto &latestPacked = latestPackedOpt->get();
            auto latestSnapshot = latestPacked.data.unpack();
            if (timestamp >= latestPacked.timestamp) return latestSnapshot;

            bool first = true;
            for (const auto &[sectionData, deltaTimestamp] : reverseDeltas) {
                if (first) {
                    first = false;
                    continue;
                }
                const auto unpacked = sectionData.unpack();
                for (size_t j = 0; j < unpackedSize; ++j) {
                    if (const auto state = unpacked[j]; state != STATE_UNCHANGED)
                        latestSnapshot[j] = state;
                }
                if (timestamp >= deltaTimestamp) break;
            }
            return latestSnapshot;
        }

        [[nodiscard]]
        auto insertSnapshot(const PackedSnapshot<unpackedSize> &newSnapshot) -> DeltaInsertionResult {
            const auto latest = latestSnapshot();
            if (!latest.has_value()) {
                reverseDeltas.push_back(newSnapshot);
                return unpackedSize;
            }
            const auto &[sectionData, timestamp] = latest.value().get();

            if (newSnapshot.timestamp <= timestamp)
                return ERR(DeltaInsertionStatus::SNAPSHOT_OLDER_THAN_LATEST);

            UnpackedData<unpackedSize> deltaSnapshotBuilder;
            const auto previousUnpacked = sectionData.unpack();
            const auto newUnpacked = newSnapshot.data.unpack();

            size_t changes = 0;
            for (size_t i = 0; i < unpackedSize; ++i) {
                const auto previous = previousUnpacked[i];
                const bool changed = newUnpacked[i] != previous;
                deltaSnapshotBuilder[i] = changed ? previous : STATE_UNCHANGED;

                if (changed) changes++;
            }
            if (changes == 0)
                return ERR(DeltaInsertionStatus::NO_CHANGES_MADE);

            const auto deltaSnapshot = PackedSnapshot {
                PackedData<unpackedSize>::pack(deltaSnapshotBuilder),
                timestamp
            };
            reverseDeltas.erase(reverseDeltas.begin());
            reverseDeltas.insert(reverseDeltas.begin(), deltaSnapshot);
            reverseDeltas.insert(reverseDeltas.begin(), newSnapshot);
            return changes;
        }
    };

}
