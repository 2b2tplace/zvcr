#pragma once

#include <zvcr/definitions.hpp>
#include <zvcr/time_utils.hpp>
#include <absl/hash/hash.h>
#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <cassert>
#include <tuple>
#include <variant>

namespace zvcr {

    using LongArray = std::vector<uint64_t>;

    struct [[deprecated]] BitStorage {

        [[deprecated]]
        BitStorage() = default;

        [[deprecated]]
        BitStorage(size_t bits, size_t size);

        [[deprecated]]
        auto init() -> void;

        [[nodiscard]]
        [[deprecated]]
        auto cellIndex(uint64_t index) const -> size_t;

        [[nodiscard]]
        [[deprecated]]
        auto get(size_t index) const -> uint64_t;

        [[deprecated]]
        auto set(size_t index, uint64_t value) -> void;

        LongArray data{};
        uint16_t packedLength{};
        size_t bits{};
        size_t size{};
        uint64_t mask{};
        size_t valuesPerLong{};
        uint64_t divideMul{};
        uint64_t divideAdd{};
        int32_t divideShift{};
    };

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

    [[nodiscard]]
    inline auto bitsPerEntryLegacy(const size_t length) -> size_t {
        return std::max<size_t>(std::bit_width(std::max(static_cast<size_t>(length), static_cast<size_t>(1)) - 1), 1UL);
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
        explicit PalettedData(const Palette &palette):
            bitStorageLegacy(palette.bitsPerEntry, unpackedSize) {
            const auto valuesPerLong = 64 / palette.bitsPerEntry;
            this->packedLongArray.resize((unpackedSize + valuesPerLong - 1) / valuesPerLong);
            this->palette = palette;
        }

        explicit PalettedData(const Palette &palette, const LongArray &packedLongArray):
            PalettedData(palette) {
            this->packedLongArray = packedLongArray;
            this->bitStorageLegacy.data = packedLongArray;
        }

        PalettedData() = default;

        [[deprecated]]
        BitStorage bitStorageLegacy;
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
            const auto legacyUnpack = palette.bitsPerEntry != 4 && palette.bitsPerEntry != 8 && palette.bitsPerEntry != 16;
            if (legacyUnpack) {
                const auto &bitStorage = palettedData.bitStorageLegacy;

                const auto shift = bitStorage.divideShift + 32;
                const uint8_t usableBits = 64 - bitStorage.bits;

                for (int64_t cellIdx = 0; cellIdx < bitStorage.packedLength; cellIdx++) {
                    const auto cell = bitStorage.data[cellIdx];
                    auto i = (cellIdx << shift) / bitStorage.divideMul;
                    for (uint8_t bitIndex = 0; bitIndex <= usableBits && i < unpackedSize; bitIndex += bitStorage.bits) {
                        auto slice = cell >> bitIndex & bitStorage.mask;
                        if (!palette.direct()) {
                            assert(slice < palette.length() && "Palette slice out of bounds");
                            slice = palette.palette[slice];
                        }
                        unpacked[i++] = slice;
                    }
                }
            } else {
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

    using magic_tuple = std::tuple<int32_t, int32_t, int32_t>;

    [[deprecated]]
    static constexpr std::array MAGIC = {
        magic_tuple{-1, -1, 0},
        magic_tuple{-2147483648, 0, 0},
        magic_tuple{1431655765, 1431655765, 0},
        magic_tuple{-2147483648, 0, 1},
        magic_tuple{858993459, 858993459, 0},
        magic_tuple{715827882, 715827882, 0},
        magic_tuple{613566756, 613566756, 0},
        magic_tuple{-2147483648, 0, 2},
        magic_tuple{477218588, 477218588, 0},
        magic_tuple{429496729, 429496729, 0},
        magic_tuple{390451572, 390451572, 0},
        magic_tuple{357913941, 357913941, 0},
        magic_tuple{330382099, 330382099, 0},
        magic_tuple{306783378, 306783378, 0},
        magic_tuple{286331153, 286331153, 0},
        magic_tuple{-2147483648, 0, 3},
        magic_tuple{252645135, 252645135, 0},
        magic_tuple{238609294, 238609294, 0},
        magic_tuple{226050910, 226050910, 0},
        magic_tuple{214748364, 214748364, 0},
        magic_tuple{204522252, 204522252, 0},
        magic_tuple{195225786, 195225786, 0},
        magic_tuple{186737708, 186737708, 0},
        magic_tuple{178956970, 178956970, 0},
        magic_tuple{171798691, 171798691, 0},
        magic_tuple{165191049, 165191049, 0},
        magic_tuple{159072862, 159072862, 0},
        magic_tuple{153391689, 153391689, 0},
        magic_tuple{148102320, 148102320, 0},
        magic_tuple{143165576, 143165576, 0},
        magic_tuple{138547332, 138547332, 0},
        magic_tuple{-2147483648, 0, 4},
        magic_tuple{130150524, 130150524, 0},
        magic_tuple{126322567, 126322567, 0},
        magic_tuple{122713351, 122713351, 0},
        magic_tuple{119304647, 119304647, 0},
        magic_tuple{116080197, 116080197, 0},
        magic_tuple{113025455, 113025455, 0},
        magic_tuple{110127366, 110127366, 0},
        magic_tuple{107374182, 107374182, 0},
        magic_tuple{104755299, 104755299, 0},
        magic_tuple{102261126, 102261126, 0},
        magic_tuple{99882960, 99882960, 0},
        magic_tuple{97612893, 97612893, 0},
        magic_tuple{95443717, 95443717, 0},
        magic_tuple{93368854, 93368854, 0},
        magic_tuple{91382282, 91382282, 0},
        magic_tuple{89478485, 89478485, 0},
        magic_tuple{87652393, 87652393, 0},
        magic_tuple{85899345, 85899345, 0},
        magic_tuple{84215045, 84215045, 0},
        magic_tuple{82595524, 82595524, 0},
        magic_tuple{81037118, 81037118, 0},
        magic_tuple{79536431, 79536431, 0},
        magic_tuple{78090314, 78090314, 0},
        magic_tuple{76695844, 76695844, 0},
        magic_tuple{75350303, 75350303, 0},
        magic_tuple{74051160, 74051160, 0},
        magic_tuple{72796055, 72796055, 0},
        magic_tuple{71582788, 71582788, 0},
        magic_tuple{70409299, 70409299, 0},
        magic_tuple{69273666, 69273666, 0},
        magic_tuple{68174084, 68174084, 0},
        magic_tuple{-2147483648, 0, 5}
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
