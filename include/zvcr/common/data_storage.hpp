#pragma once

#include <absl/container/flat_hash_map.h>
#include <zvcr/common/definitions.hpp>
#include <zvcr/common/result.hpp>

namespace zvcr::paletted_storage {

    using LongArray = std::vector<uint64_t>;
    using namespace definitions;

    template<typename T>
    class UnpackedView;

    class BitStorage {
    public:
        LongArray data;

        BitStorage(size_t bits, size_t size, const LongArray& data = LongArray(0));

        [[nodiscard]]
        size_t cellIndex(uint64_t index) const;

        [[nodiscard]]
        uint64_t get(size_t index) const;

        void set(size_t index, uint64_t value);
    private:
        size_t bits;
        size_t size;
        uint64_t mask;
        size_t valuesPerLong;
        uint64_t divideMul;
        uint64_t divideAdd;
        int32_t divideShift;
    };

    template<typename T>
    class PackedData {
    public:
        using UnpackedData = std::vector<T>;

        explicit PackedData(const UnpackedData& palette, LongArray packedData, const size_t snapshotLength): packedData(std::move(packedData)) {
            this->snapshotLength = snapshotLength;
            this->palette = palette;
            this->bitsPerIndex = getBitsPerIndex(palette);
        }

        [[nodiscard]]
        static uint64_t getBitsPerIndex(const UnpackedData& palette) {
            return std::max(static_cast<int>(std::ceil(log2(static_cast<double>(palette.size())))), 1);
        }

        [[nodiscard]]
        static PackedData pack(const UnpackedData& sectionData) {
            const auto snapshotLength = sectionData.size();

            std::vector<T> palette;
            palette.reserve(snapshotLength);

            absl::flat_hash_map<T, size_t> stateToIndex;
            stateToIndex.reserve(snapshotLength);

            for (const auto& item : sectionData) {
                if (stateToIndex.contains(item)) continue;

                palette.emplace_back(item);
                stateToIndex[item] = 0;
            }
            std::ranges::sort(palette);

            for (size_t i = 0; i < palette.size(); ++i)
                stateToIndex[palette.at(i)] = i;

            auto bitStorage = BitStorage(getBitsPerIndex(palette), snapshotLength);

            for (size_t i = 0; i < snapshotLength; ++i)
                bitStorage.set(i, stateToIndex.at(sectionData.at(i)));

            return PackedData{palette, bitStorage.data, snapshotLength};
        }

        [[nodiscard]]
        UnpackedData unpack() const {
            UnpackedData unpacked(snapshotLength);
            const BitStorage bitStorage(bitsPerIndex, snapshotLength, packedData);

            size_t index = 0;
            for (size_t i = 0; i < snapshotLength; ++i) {
                const auto slice = bitStorage.get(i);
                unpacked[index] = palette[slice];
                ++index;
            }
            return unpacked;
        }

        [[nodiscard]]
        UnpackedView<T> view(const uint8_t sidelength) const {
            return UnpackedView{sidelength, unpack()};
        }

        size_t snapshotLength;
        LongArray packedData;
        UnpackedData palette;
        uint64_t bitsPerIndex;
    };

}

namespace zvcr::reverse_delta {

    template<typename T>
    struct PackedSnapshot {
        paletted_storage::PackedData<T> data;
        time_t timestamp{};
    };

}

namespace zvcr::paletted_storage {

    template<typename T>
    class UnpackedView {
    public:
        using UnpackedData = std::vector<T>;

        UnpackedData unpacked;

        explicit UnpackedView(const uint8_t sidelength, size_t snapshotLength): sidelength(sidelength) {
            this->unpacked = std::vector<T>(snapshotLength);
        }

        explicit UnpackedView(const uint8_t sidelength, size_t snapshotLength, T fill): sidelength(sidelength) {
            this->unpacked = std::vector<T>(snapshotLength, fill);
        }

        explicit UnpackedView(const uint8_t sidelength, const UnpackedData& unpacked): sidelength(sidelength) {
            this->unpacked = unpacked;
        }

        [[nodiscard]]
        T getVoxel(const uint8_t x, const uint8_t y, const uint8_t z) const {
            return unpacked[unpackedIndex(x, y, z)];
        }

        void setVoxel(const uint8_t x, const uint8_t y, const uint8_t z, T voxel) {
            unpacked[unpackedIndex(x, y, z)] = voxel;
        }

        [[nodiscard]]
        T getPixel(const uint8_t x, const uint8_t z) const {
            return unpacked[unpackedIndex(x, z)];
        }

        void setPixel(const uint8_t x, const uint8_t z, T pixel) {
            unpacked[unpackedIndex(x, z)] = pixel;
        }

        [[nodiscard]]
        PackedData<T> pack() const {
            return PackedData<T>::pack(unpacked);
        }

        [[nodiscard]]
        reverse_delta::PackedSnapshot<T> packSnapshot(time_t timestamp) const {
            return reverse_delta::PackedSnapshot{pack(), timestamp};
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

        [[nodiscard]]
        static UnpackedView create2DBlockView(T fill) {
            return UnpackedView{SEGMENT_SIDELENGTH_BLOCKS, SECTION_2D_SIZE_BLOCKS, fill};
        }

        [[nodiscard]]
        static UnpackedView create3DBlockView(T fill) {
            return UnpackedView{SEGMENT_SIDELENGTH_BLOCKS, SECTION_3D_SIZE_BLOCKS, fill};
        }

        [[nodiscard]]
        static UnpackedView create2DBiomeView(T fill) {
            return UnpackedView{SEGMENT_SIDELENGTH_BIOMES, SECTION_2D_SIZE_BIOMES, fill};
        }

        [[nodiscard]]
        static UnpackedView create3DBiomeView(T fill) {
            return UnpackedView{SEGMENT_SIDELENGTH_BIOMES, SECTION_3D_SIZE_BIOMES, fill};
        }

        [[nodiscard]]
        static UnpackedView create2DBlockView() {
            return create2DBlockView(0);
        }

        [[nodiscard]]
        static UnpackedView create3DBlockView() {
            return create3DBlockView(0);
        }

        [[nodiscard]]
        static UnpackedView create2DBiomeView() {
            return create2DBiomeView(0);
        }

        [[nodiscard]]
        static UnpackedView create3DBiomeView() {
            return create3DBiomeView(0);
        }
    private:
        uint8_t sidelength;
    };

    using magic_tuple = std::tuple<int64_t, int64_t, int32_t>;

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

}

namespace zvcr::reverse_delta {

    using paletted_storage::PackedData;
    using namespace result;

    static constexpr uint16_t STATE_UNCHANGED = 0xFFFF;

    enum class DeltaInsertionStatus {
        INVALID_SNAPSHOT_LENGTH,
        SNAPSHOT_OLDER_THAN_LATEST,
        NO_CHANGES_MADE
    };

    using DeltaInsertionResult = Result<size_t, DeltaInsertionStatus>;

    template<typename T, T Unchanged = STATE_UNCHANGED>
    class PackedDeltaData {
    public:
        size_t snapshotLength;
        std::vector<PackedSnapshot<T>> reverseDeltas;

        explicit PackedDeltaData(const PackedSnapshot<T>& initialState) {
            this->snapshotLength = initialState.data.snapshotLength;
            this->reverseDeltas.push_back(initialState);
        }

        explicit PackedDeltaData(const std::vector<PackedSnapshot<T>>& reverseDeltas, size_t snapshotLength) {
            this->snapshotLength = snapshotLength;
            this->reverseDeltas = reverseDeltas;
        }

        explicit PackedDeltaData(size_t snapshotLength):
            PackedDeltaData({}, snapshotLength) {}

        [[nodiscard]]
        OptionCRef<PackedSnapshot<T>> latestSnapshot() const {
            return delta(0);
        }

        [[nodiscard]]
        OptionCRef<PackedSnapshot<T>> delta(size_t deltaIndex) const {
            return reverseDeltas.empty() ? OptionCRef<PackedSnapshot<T>>{} : OptionCRef{reverseDeltas[deltaIndex]};
        }

        [[nodiscard]]
        Option<PackedSnapshot<T>> snapshotFrom(time_t timestamp) const {
            const auto latest = Require(this->latestSnapshot());

            auto latestSnapshot = latest.data.unpack();
            bool first = true;
            for (const auto& [sectionData, deltaTimestamp] : reverseDeltas) {
                if (first) {
                    first = false;
                    continue;
                }
                if (timestamp > deltaTimestamp) break;

                const auto unpacked = sectionData.unpack();
                for (size_t j = 0; j < snapshotLength; ++j) {
                    if (const auto state = unpacked[j]; state != Unchanged)
                        latestSnapshot[j] = state;
                }
            }
            return PackedSnapshot{PackedData<T>::pack(latestSnapshot), timestamp};
        }

        [[nodiscard]]
        DeltaInsertionResult insertSnapshot(const PackedSnapshot<T>& newSnapshot) {
            const auto latest = latestSnapshot();
            if (latest.none()) {
                reverseDeltas.push_back(newSnapshot);
                return newSnapshot.data.snapshotLength;
            }
            if (newSnapshot.data.snapshotLength != this->snapshotLength)
                return Err(DeltaInsertionStatus::INVALID_SNAPSHOT_LENGTH);

            const auto& [sectionData, timestamp] = latest.unwrap();

            if (newSnapshot.timestamp <= timestamp)
                return Err(DeltaInsertionStatus::SNAPSHOT_OLDER_THAN_LATEST);

            std::vector<T> deltaSnapshotBuilder(snapshotLength);

            const auto previousUnpacked = sectionData.unpack();
            const auto newUnpacked = newSnapshot.data.unpack();

            size_t changes = 0;
            for (size_t i = 0; i < snapshotLength; ++i) {
                const auto previous = previousUnpacked[i];
                const bool changed = newUnpacked[i] != previous;
                deltaSnapshotBuilder[i] = changed ? previous : Unchanged;

                if (changed) changes++;
            }
            if (changes == 0)
                return Err(DeltaInsertionStatus::NO_CHANGES_MADE);

            const auto deltaSnapshot = PackedSnapshot {
                PackedData<T>::pack(deltaSnapshotBuilder),
                timestamp
            };
            reverseDeltas.erase(reverseDeltas.begin());
            reverseDeltas.insert(reverseDeltas.begin(), deltaSnapshot);
            reverseDeltas.insert(reverseDeltas.begin(), newSnapshot);
            return changes;
        }
    };

}
