#pragma once

#include <zvcr/common/definitions.hpp>
#include <result.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <cassert>
#include <tuple>
#include <variant>

namespace zvcr {

    using LongArray = std::vector<uint64_t>;

    struct BitStorage {

        BitStorage() = default;

        BitStorage(size_t bits, size_t size);

        void init();

        [[nodiscard]]
        size_t cellIndex(uint64_t index) const;

        [[nodiscard]]
        uint64_t get(size_t index) const;

        void set(size_t index, uint64_t value);

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

    template<size_t snapshotLength>
    using UnpackedData = std::array<SegmentAtom, snapshotLength>;

    template<size_t>
    struct PackedSnapshot;

    template<size_t snapshotLength>
    class UnpackedView {
    public:
        UnpackedData<snapshotLength> unpacked;

        explicit UnpackedView(const uint8_t sidelength): sidelength(sidelength) {
            this->unpacked = UnpackedData<snapshotLength>{};
        }

        explicit UnpackedView(const uint8_t sidelength, const SegmentAtom fill): sidelength(sidelength) {
            this->unpacked = UnpackedData<snapshotLength>{};
            this->unpacked.fill(fill);
        }

        explicit UnpackedView(const uint8_t sidelength, const UnpackedData<snapshotLength>& unpacked): sidelength(sidelength) {
            this->unpacked = unpacked;
        }

        [[nodiscard]]
        SegmentAtom getVoxel(const uint8_t x, const uint8_t y, const uint8_t z) const {
            return unpacked[unpackedIndex(x, y, z)];
        }

        void setVoxel(const uint8_t x, const uint8_t y, const uint8_t z, const SegmentAtom voxel) {
            unpacked[unpackedIndex(x, y, z)] = voxel;
        }

        [[nodiscard]]
        SegmentAtom getPixel(const uint8_t x, const uint8_t z) const {
            return unpacked[unpackedIndex(x, z)];
        }

        void setPixel(const uint8_t x, const uint8_t z, const SegmentAtom pixel) {
            unpacked[unpackedIndex(x, z)] = pixel;
        }

        [[nodiscard]]
        PackedData<snapshotLength> pack() const {
            return PackedData<snapshotLength>::pack(unpacked);
        }

        [[nodiscard]]
        PackedSnapshot<snapshotLength> packSnapshot(time_t timestamp) const {
            return PackedSnapshot{pack(), timestamp};
        }

        [[nodiscard]]
        size_t unpackedIndex(const uint8_t x, const uint8_t y, const uint8_t z) const {
            assert(x < sidelength && "X coordinate out of bounds");
            assert(y < sidelength && "Y coordinate out of bounds");
            assert(z < sidelength && "Z coordinate out of bounds");

            return static_cast<size_t>(y) * sidelength * sidelength
                 + static_cast<size_t>(z) * sidelength
                 + static_cast<size_t>(x);
        }

        [[nodiscard]]
        uint8_t getSidelength() const {
            return sidelength;
        }

        [[nodiscard]]
        size_t unpackedIndex(const uint8_t x, const uint8_t z) const {
            return unpackedIndex(x, 0, z);
        }
    private:
        uint8_t sidelength;
    };

    [[nodiscard]]
    inline UnpackedView<SECTION_2D_SIZE_BLOCKS> create2DBlockView(const SegmentAtom fill) {
        return UnpackedView<SECTION_2D_SIZE_BLOCKS>{SEGMENT_SIDELENGTH_BLOCKS, fill};
    }

    [[nodiscard]]
    inline UnpackedView<SECTION_3D_SIZE_BLOCKS> create3DBlockView(const SegmentAtom fill) {
        return UnpackedView<SECTION_3D_SIZE_BLOCKS>{SEGMENT_SIDELENGTH_BLOCKS, fill};
    }

    [[nodiscard]]
    inline UnpackedView<SECTION_2D_SIZE_BIOMES> create2DBiomeView(const SegmentAtom fill) {
        return UnpackedView<SECTION_2D_SIZE_BIOMES>{SEGMENT_SIDELENGTH_BIOMES, fill};
    }

    [[nodiscard]]
    inline UnpackedView<SECTION_3D_SIZE_BIOMES> create3DBiomeView(const SegmentAtom fill) {
        return UnpackedView<SECTION_3D_SIZE_BIOMES>{SEGMENT_SIDELENGTH_BIOMES, fill};
    }

    [[nodiscard]]
    inline UnpackedView<SECTION_2D_SIZE_BLOCKS> create2DBlockView() {
        return create2DBlockView(0);
    }

    [[nodiscard]]
    inline UnpackedView<SECTION_3D_SIZE_BLOCKS> create3DBlockView() {
        return create3DBlockView(0);
    }

    [[nodiscard]]
    inline UnpackedView<SECTION_2D_SIZE_BIOMES> create2DBiomeView() {
        return create2DBiomeView(0);
    }

    [[nodiscard]]
    inline UnpackedView<SECTION_3D_SIZE_BIOMES> create3DBiomeView() {
        return create3DBiomeView(0);
    }

    static constexpr size_t MAX_PALETTE_SIZE = UINT8_MAX + 1;

    using VectorPalette = std::vector<SegmentAtom>;

    struct Palette {
        VectorPalette palette{};
        size_t length{};
        uint64_t bitsPerIndex{};

        [[nodiscard]]
        static uint64_t getBitsPerIndex(const size_t length) {
            return std::max<uint64_t>(std::bit_width(std::max(static_cast<uint64_t>(length), static_cast<uint64_t>(1)) - 1), 1UL);
        }

        [[nodiscard]]
        bool equals(const Palette &other) const {
            if (length != other.length) return false;

            return std::equal(palette.begin(), palette.begin() + static_cast<ssize_t>(length), other.palette.begin());
        }

        [[nodiscard]]
        bool direct() const {
            return length == 0;
        }
    };

    static const auto DIRECT_PALETTE = Palette{.bitsPerIndex = 16};

    template<size_t snapshotLength>
    [[nodiscard]]
    Palette buildPalette(const UnpackedData<snapshotLength> &data, std::array<uint8_t, UINT16_MAX + 1> &indices) {
        Palette palette;
        std::bitset<UINT16_MAX + 1> unique;
        for (const auto atom : data) {
            if (unique.test(atom)) continue;
            unique.set(atom);

            if (palette.length >= MAX_PALETTE_SIZE)
                return DIRECT_PALETTE;

            if (palette.length >= palette.palette.size())
                palette.palette.resize(MAX_PALETTE_SIZE);

            palette.palette[palette.length] = atom;
            indices[atom] = static_cast<uint16_t>(palette.length++);
        }
        palette.bitsPerIndex = Palette::getBitsPerIndex(palette.length);
        return palette;
    }

    template<size_t snapshotLength>
    struct PalettedData {
        explicit PalettedData(const Palette& palette):
            bitStorage(palette.bitsPerIndex, snapshotLength) {
            this->palette = palette;
        }

        explicit PalettedData(const Palette& palette, const LongArray &packedData):
            PalettedData(palette) {
            this->bitStorage.data = packedData;
        }

        PalettedData() = default;

        BitStorage bitStorage;
        Palette palette;
    };

    template<size_t snapshotLength>
    using Data = std::variant<PalettedData<snapshotLength>, uint16_t>;

    template<size_t snapshotLength>
    class PackedData {
    public:
        explicit PackedData(const Palette& palette):
            data(PalettedData<snapshotLength>{palette}) {}

        explicit PackedData(const Palette& palette, const LongArray &packedData):
            data(PalettedData<snapshotLength>{palette, packedData}) {}

        explicit PackedData(const PalettedData<snapshotLength> &palettedData):
            data(palettedData) {}

        explicit PackedData(const uint16_t singleValue):
            data(singleValue) {}

        PackedData() = default;

        [[nodiscard]]
        static PackedData pack(const UnpackedData<snapshotLength>& sectionData) {
            std::array<uint8_t, UINT16_MAX + 1> indices{};
            const auto palette = buildPalette(sectionData, indices);
            if (palette.length == 1)
                return PackedData{palette.palette[0]};

            auto packedData = PackedData{palette};
            auto &palettedData = std::get<PalettedData<snapshotLength>>(packedData.data);
            auto &bitStorage = palettedData.bitStorage;

            const auto shift = bitStorage.divideShift + 32;
            const uint8_t usableBits = 64 - bitStorage.bits;

            for (int64_t cellIdx = 0; cellIdx < bitStorage.packedLength; cellIdx++) {
                auto i = (cellIdx << shift) / bitStorage.divideMul;
                auto cell = 0UL;
                for (uint8_t bitIndex = 0; bitIndex <= usableBits && i < snapshotLength; bitIndex += bitStorage.bits) {
                    auto value = sectionData[i];
                    if (!palette.direct()) {
                        assert(value < indices.size() && "Palette index out of bounds");
                        value = indices[value];
                    }
                    cell = cell & ~(bitStorage.mask << bitIndex) | (value & bitStorage.mask) << bitIndex;
                    ++i;
                }
                bitStorage.data[cellIdx] = cell;
            }
            return packedData;
        }

        [[nodiscard]]
        UnpackedData<snapshotLength> unpack() const {
            if (std::holds_alternative<uint16_t>(data)) {
                auto result = UnpackedData<snapshotLength>{};
                result.fill(std::get<uint16_t>(data));
                return result;
            }
            UnpackedData<snapshotLength> unpacked{};
            const auto &palettedData = std::get<PalettedData<snapshotLength>>(data);
            const auto &bitStorage = palettedData.bitStorage;
            const auto &palette = palettedData.palette;

            const auto shift = bitStorage.divideShift + 32;
            const uint8_t usableBits = 64 - bitStorage.bits;

            for (int64_t cellIdx = 0; cellIdx < bitStorage.packedLength; cellIdx++) {
                const auto cell = bitStorage.data[cellIdx];
                auto i = (cellIdx << shift) / bitStorage.divideMul;
                for (uint8_t bitIndex = 0; bitIndex <= usableBits && i < snapshotLength; bitIndex += bitStorage.bits) {
                    auto slice = cell >> bitIndex & bitStorage.mask;
                    if (!palette.direct()) {
                        assert(slice < palette.length && "Palette slice out of bounds");
                        slice = palette.palette[slice];
                    }
                    unpacked[i++] = slice;
                }
            }
            return unpacked;
        }

        [[nodiscard]]
        UnpackedView<snapshotLength> view(const uint8_t sidelength) const {
            return UnpackedView{sidelength, unpack()};
        }

        Data<snapshotLength> data;
    };

    template<size_t snapshotLength>
    struct PackedSnapshot {
        PackedData<snapshotLength> data;
        time_t timestamp{};
    };

    using magic_tuple = std::tuple<int32_t, int32_t, int32_t>;

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

    static constexpr uint16_t STATE_UNCHANGED = 0xFFFF;

    enum class DeltaInsertionStatus {
        SNAPSHOT_OLDER_THAN_LATEST,
        NO_CHANGES_MADE
    };

    using DeltaInsertionResult = result::Result<size_t, DeltaInsertionStatus>;

    template<size_t snapshotLength>
    using PackedSnapshotVector = std::vector<PackedSnapshot<snapshotLength>>;

    template<size_t snapshotLength>
    class PackedDeltaData {
    public:
        PackedSnapshotVector<snapshotLength> reverseDeltas;

        explicit PackedDeltaData(const PackedSnapshot<snapshotLength>& initialState) {
            this->reverseDeltas.push_back(initialState);
        }

        explicit PackedDeltaData(PackedSnapshotVector<snapshotLength> reverseDeltas):
            reverseDeltas(std::move(reverseDeltas)) {}

        PackedDeltaData(): reverseDeltas() {}

        [[nodiscard]]
        result::OptionCRef<PackedSnapshot<snapshotLength>> latestSnapshot() const {
            return delta(0);
        }

        [[nodiscard]]
        result::OptionCRef<PackedSnapshot<snapshotLength>> delta(const size_t deltaIndex) const {
            return reverseDeltas.empty() ? result::None : result::OptionCRef<PackedSnapshot<snapshotLength>>{reverseDeltas[deltaIndex]};
        }

        [[nodiscard]]
        result::Option<UnpackedData<snapshotLength>> snapshotFrom(const time_t timestamp) const {
            const auto &latestPackedOpt = this->latestSnapshot();
            if (!latestPackedOpt) return result::None;

            const auto &latestPacked = REQUIRE(latestPackedOpt).get();
            auto latestSnapshot = latestPacked.data.unpack();
            if (timestamp >= latestPacked.timestamp) return latestSnapshot;

            bool first = true;
            for (const auto& [sectionData, deltaTimestamp] : reverseDeltas) {
                if (first) {
                    first = false;
                    continue;
                }
                const auto unpacked = sectionData.unpack();
                for (size_t j = 0; j < snapshotLength; ++j) {
                    if (const auto state = unpacked[j]; state != STATE_UNCHANGED)
                        latestSnapshot[j] = state;
                }
                if (timestamp >= deltaTimestamp) break;
            }
            return latestSnapshot;
        }

        [[nodiscard]]
        DeltaInsertionResult insertSnapshot(const PackedSnapshot<snapshotLength>& newSnapshot) {
            const auto latest = latestSnapshot();
            if (!latest.has_value()) {
                reverseDeltas.push_back(newSnapshot);
                return snapshotLength;
            }
            const auto& [sectionData, timestamp] = latest.value().get();

            if (newSnapshot.timestamp <= timestamp)
                return ERR(DeltaInsertionStatus::SNAPSHOT_OLDER_THAN_LATEST);

            UnpackedData<snapshotLength> deltaSnapshotBuilder;
            const auto previousUnpacked = sectionData.unpack();
            const auto newUnpacked = newSnapshot.data.unpack();

            size_t changes = 0;
            for (size_t i = 0; i < snapshotLength; ++i) {
                const auto previous = previousUnpacked[i];
                const bool changed = newUnpacked[i] != previous;
                deltaSnapshotBuilder[i] = changed ? previous : STATE_UNCHANGED;

                if (changed) changes++;
            }
            if (changes == 0)
                return ERR(DeltaInsertionStatus::NO_CHANGES_MADE);

            const auto deltaSnapshot = PackedSnapshot {
                PackedData<snapshotLength>::pack(deltaSnapshotBuilder),
                timestamp
            };
            reverseDeltas.erase(reverseDeltas.begin());
            reverseDeltas.insert(reverseDeltas.begin(), deltaSnapshot);
            reverseDeltas.insert(reverseDeltas.begin(), newSnapshot);
            return changes;
        }
    };

}
