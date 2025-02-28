#include <zvcr/common/data_storage.hpp>

#include <utility>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <tuple>
#include <unordered_set>
#include <absl/container/flat_hash_map.h>
#include <zvcr/common/definitions.hpp>

namespace zvcr::common::reverse_delta {

    template<typename T>
    PackedDeltaData<T>::PackedDeltaData(const size_t snapshotLength) {
        this->snapshotLength = snapshotLength;
        this->reverseDeltas = {};
    }

    template<typename T>
    PackedDeltaData<T>::PackedDeltaData(const PackedSnapshot<T>& initialState) {
        this->snapshotLength = initialState.data.snapshotLength;
        this->reverseDeltas.push_back(initialState);
    }

    template<typename T>
    PackedDeltaData<T>::PackedDeltaData(const std::vector<PackedSnapshot<T>>& reverseDeltas, const size_t snapshotLength) {
        this->snapshotLength = snapshotLength;
        this->reverseDeltas = reverseDeltas;
    }

    template<typename T>
    OptionCRef<PackedSnapshot<T>> PackedDeltaData<T>::latestSnapshot() const {
        return delta(0);
    }

    template<typename T>
    OptionCRef<PackedSnapshot<T>> PackedDeltaData<T>::delta(const size_t deltaIndex) const {
        return reverseDeltas.empty() ? OptionCRef<PackedSnapshot<T>>() : OptionCRef(reverseDeltas[deltaIndex]);
    }

    template<typename T>
    Option<PackedSnapshot<T>> PackedDeltaData<T>::snapshotFrom(const time_t timestamp) const {
        const auto latest = this->latestSnapshot();
        if (!latest.hasSome()) return {};

        auto latestSnapshot = latest->data.unpack();
        bool first = true;
        for (const auto& [sectionData, deltaTimestamp] : reverseDeltas) {
            if (first) {
                first = false;
                continue;
            }
            if (timestamp > deltaTimestamp) break;

            const auto unpacked = sectionData.unpack();
            for (size_t j = 0; j < snapshotLength; ++j) {
                if (const auto state = unpacked[j]; state != STATE_UNCHANGED)
                    latestSnapshot[j] = state;
            }
        }
        return PackedSnapshot {PackedData<T>::pack(latestSnapshot), timestamp};
    }

    template<typename T>
    DeltaInsertionResult PackedDeltaData<T>::insertSnapshot(const PackedSnapshot<T>& newSnapshot) {
        const auto latest = latestSnapshot();
        if (!latest.hasSome()) {
            reverseDeltas.push_back(newSnapshot);
            return newSnapshot.data.snapshotLength;
        }
        if (newSnapshot.data.snapshotLength != this->snapshotLength)
            return Error(DeltaInsertionStatus::INVALID_SNAPSHOT_LENGTH);

        const auto& [sectionData, timestamp] = latest.unwrap();

        if (newSnapshot.timestamp <= timestamp)
            return Error(DeltaInsertionStatus::SNAPSHOT_OLDER_THAN_LATEST);

        std::vector<T> deltaSnapshotBuilder(snapshotLength);

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
            return Error(DeltaInsertionStatus::NO_CHANGES_MADE);

        const auto deltaSnapshot = PackedSnapshot {
            PackedData<T>::pack(deltaSnapshotBuilder),
            timestamp
        };
        reverseDeltas.erase(reverseDeltas.begin());
        reverseDeltas.insert(reverseDeltas.begin(), deltaSnapshot);
        reverseDeltas.insert(reverseDeltas.begin(), newSnapshot);
        return changes;
    }

}

namespace zvcr::common::paletted_storage {

    using namespace definitions;

    template<typename T>
    UnpackedView<T>::UnpackedView(const size_t snapshotLength) {
        this->unpacked = std::vector<T>(snapshotLength);
    }

    template<typename T>
    UnpackedView<T>::UnpackedView(const size_t snapshotLength, const T fill) {
        this->unpacked = std::vector<T>(snapshotLength, fill);
    }

    template<typename T>
    UnpackedView<T>::UnpackedView(const std::vector<T>& unpacked) {
        this->unpacked = unpacked;
    }

    template<typename T>
    T UnpackedView<T>::getVoxel(const uint8_t x, const uint8_t y, const uint8_t z) const {
        return unpacked[unpackedIndex(x, y, z)];
    }

    template<typename T>
    void UnpackedView<T>::setVoxel(const uint8_t x, const uint8_t y, const uint8_t z, const T voxel) {
        unpacked[unpackedIndex(x, y, z)] = voxel;
    }

    template<typename T>
    size_t UnpackedView<T>::unpackedIndex(const uint8_t x, const uint8_t y, const uint8_t z) {
        assert(x < SEGMENT_SIDELENGTH_BLOCKS);
        assert(y < SEGMENT_SIDELENGTH_BLOCKS);
        assert(z < SEGMENT_SIDELENGTH_BLOCKS);

        return static_cast<size_t>(y) * SEGMENT_SIDELENGTH_BLOCKS * SEGMENT_SIDELENGTH_BLOCKS
             + static_cast<size_t>(z) * SEGMENT_SIDELENGTH_BLOCKS
             + static_cast<size_t>(x);
    }

    template<typename T>
    T UnpackedView<T>::getPixel(const uint8_t x, const uint8_t z) const {
        return unpacked[unpackedIndex(x, z)];
    }

    template<typename T>
    void UnpackedView<T>::setPixel(const uint8_t x, const uint8_t z, const T pixel) {
        unpacked[unpackedIndex(x, z)] = pixel;
    }

    template<typename T>
    PackedData<T> UnpackedView<T>::pack() const {
        return PackedData<T>::pack(unpacked);
    }

    template<typename T>
    reverse_delta::PackedSnapshot<T> UnpackedView<T>::packSnapshot(const time_t timestamp) const {
        return reverse_delta::PackedSnapshot {pack(), timestamp};
    }

    template<typename T>
    size_t UnpackedView<T>::unpackedIndex(const uint8_t x, const uint8_t z) {
        return unpackedIndex(x, 0, z);
    }

    template<typename T>
    UnpackedView<T> UnpackedView<T>::create2DView(const T fill) {
        return {SECTION_2D_SIZE_BLOCKS, fill};
    }

    template<typename T>
    UnpackedView<T> UnpackedView<T>::create3DView(const T fill) {
        return UnpackedView {SECTION_3D_SIZE_BLOCKS, fill};
    }

    template<typename T>
    UnpackedView<T> UnpackedView<T>::create2DView() {
        return UnpackedView {SECTION_2D_SIZE_BLOCKS};
    }

    template<typename T>
    UnpackedView<T> UnpackedView<T>::create3DView() {
        return UnpackedView {SECTION_3D_SIZE_BLOCKS};
    }

    BitStorage::BitStorage(const size_t bits, const size_t size, const LongArray& data = LongArray(0)): data(data), bits(bits), // NOLINT(*-pro-type-member-init)
        size(size) {
        assert(bits >= 1 && bits <= 32);

        valuesPerLong = 64 / bits;
        const size_t magicIndex = valuesPerLong - 1;
        std::tie(divideMul, divideAdd, divideShift) = MAGIC[magicIndex];
        const size_t calculatedLength = (size + valuesPerLong - 1) / valuesPerLong;

        divideMul = static_cast<uint64_t>(static_cast<uint32_t>(divideMul));
        divideAdd = static_cast<uint64_t>(static_cast<uint32_t>(divideAdd));
        mask = (1ULL << bits) - 1;

        if (data.empty()) {
            this->data.resize(calculatedLength);
            return;
        }
        assert(data.size() == calculatedLength);
    }

    size_t BitStorage::cellIndex(const uint64_t index) const {
        return index * divideMul + divideAdd >> 32 >> divideShift;
    }

    uint64_t BitStorage::get(const size_t index) const {
        assert(index < size && "Index out of bounds");

        if (data.empty()) return 0;

        const size_t cellIdx = cellIndex(index);
        const uint64_t cell = data[cellIdx];
        const size_t bitIndex = (index - cellIdx * valuesPerLong) * bits;
        return cell >> bitIndex & mask;
    }

    void BitStorage::set(const size_t index, const uint64_t value) {
        if (data.empty()) return;

        assert(index < size);
        assert(value <= mask);

        const size_t cellIdx = cellIndex(index);
        uint64_t& cell = data[cellIdx];
        const size_t bitIndex = (index - cellIdx * valuesPerLong) * bits;
        cell = cell & ~(mask << bitIndex) | (value & mask) << bitIndex;
    }

    template<typename T>
    PackedData<T>::PackedData(const UnpackedData& palette, LongArray packedData, const size_t snapshotLength): packedData(std::move(packedData)) {
        this->snapshotLength = snapshotLength;
        this->palette = palette;
        this->bitsPerIndex = getBitsPerIndex(palette);
    }

    template<typename T>
    uint64_t PackedData<T>::getBitsPerIndex(const UnpackedData& palette) {
        return std::max(static_cast<int>(std::ceil(log2(static_cast<double>(palette.size())))), 1);
    }

    template<typename T>
    typename PackedData<T>::UnpackedData PackedData<T>::unpack() const {
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

    template<typename T>
    PackedData<T> PackedData<T>::pack(const UnpackedData& sectionData) {
        std::unordered_set uniqueStates(sectionData.begin(), sectionData.end());
        std::vector palette(uniqueStates.begin(), uniqueStates.end());

        std::ranges::sort(palette);

        absl::flat_hash_map<uint16_t, size_t> stateToIndex;
        stateToIndex.reserve(palette.size());
        for (size_t i = 0; i < palette.size(); ++i)
            stateToIndex[palette[i]] = i;

        const auto snapshotLength = sectionData.size();
        BitStorage bitStorage(getBitsPerIndex(palette), snapshotLength);

        for (size_t i = 0; i < snapshotLength; ++i)
            bitStorage.set(i, stateToIndex[sectionData[i]]);

        return PackedData(palette, bitStorage.data, snapshotLength);
    }

    template<typename T>
    UnpackedView<T> PackedData<T>::view() const {
        return UnpackedView(unpack());
    }

}