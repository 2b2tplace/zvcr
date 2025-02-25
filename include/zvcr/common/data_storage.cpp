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

    DeltaBlockStates::DeltaBlockStates(const size_t snapshotLength) {
        this->snapshotLength = snapshotLength;
        this->reverseDeltas = {};
    }

    DeltaBlockStates::DeltaBlockStates(const BlockStatesSnapshot& initialState) {
        this->snapshotLength = initialState.data.snapshotLength;
        this->reverseDeltas.push_back(initialState);
    }

    DeltaBlockStates::DeltaBlockStates(const std::vector<BlockStatesSnapshot>& reverseDeltas, const size_t snapshotLength) {
        this->snapshotLength = snapshotLength;
        this->reverseDeltas = reverseDeltas;
    }

    std::optional<BlockStatesSnapshot> DeltaBlockStates::latestSnapshot() const {
        return delta(0);
    }

    std::optional<BlockStatesSnapshot> DeltaBlockStates::delta(const size_t deltaIndex) const {
        return reverseDeltas.empty() ? std::nullopt : std::optional(reverseDeltas[deltaIndex]);
    }

    std::optional<BlockStatesSnapshot> DeltaBlockStates::snapshotFrom(const time_t timestamp) const {
        const auto latest = this->latestSnapshot();
        if (!latest.has_value()) return std::nullopt;

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
        return BlockStatesSnapshot {BlockStates::pack(latestSnapshot), timestamp};
    }

    DeltaInsertionResult DeltaBlockStates::insertSnapshot(const BlockStatesSnapshot& newSnapshot) {
        const auto latest = latestSnapshot();
        if (!latest.has_value()) {
            reverseDeltas.push_back(newSnapshot);
            return newSnapshot.data.snapshotLength;
        }
        if (newSnapshot.data.snapshotLength != this->snapshotLength)
            return result::Error(DeltaInsertionStatus::INVALID_SNAPSHOT_LENGTH);

        const auto [sectionData, timestamp] = *latest;

        if (newSnapshot.timestamp <= timestamp)
            return result::Error(DeltaInsertionStatus::SNAPSHOT_OLDER_THAN_LATEST);

        paletted_storage::UnpackedBlockStates deltaSnapshotBuilder(snapshotLength);

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
            return result::Error(DeltaInsertionStatus::NO_CHANGES_MADE);

        const auto deltaSnapshot = BlockStatesSnapshot {
            BlockStates::pack(deltaSnapshotBuilder),
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

    BlockStatesView::BlockStatesView(const size_t snapshotLength) {
        this->unpacked = UnpackedBlockStates(snapshotLength);
    }

    BlockStatesView::BlockStatesView(const size_t snapshotLength, const uint16_t fill) {
        this->unpacked = UnpackedBlockStates(snapshotLength, fill);
    }

    BlockStatesView::BlockStatesView(const UnpackedBlockStates& unpacked) {
        this->unpacked = unpacked;
    }

    uint16_t BlockStatesView::getBlockState(const uint8_t x, const uint8_t y, const uint8_t z) const {
        return unpacked[unpackedIndex(x, y, z)];
    }

    void BlockStatesView::setBlockState(const uint8_t x, const uint8_t y, const uint8_t z, const uint16_t blockStateId) {
        unpacked[unpackedIndex(x, y, z)] = blockStateId;
    }

    size_t BlockStatesView::unpackedIndex(const uint8_t x, const uint8_t y, const uint8_t z) {
        assert(x < SEGMENT_SIDELENGTH_BLOCKS);
        assert(y < SEGMENT_SIDELENGTH_BLOCKS);
        assert(z < SEGMENT_SIDELENGTH_BLOCKS);

        return static_cast<size_t>(y) * SEGMENT_SIDELENGTH_BLOCKS * SEGMENT_SIDELENGTH_BLOCKS
             + static_cast<size_t>(z) * SEGMENT_SIDELENGTH_BLOCKS
             + static_cast<size_t>(x);
    }

    uint16_t BlockStatesView::get(const uint8_t x, const uint8_t z) const {
        return unpacked[unpackedIndex(x, z)];
    }

    void BlockStatesView::set(const uint8_t x, const uint8_t z, const uint16_t blockStateId) {
        unpacked[unpackedIndex(x, z)] = blockStateId;
    }

    BlockStates BlockStatesView::pack() const {
        return BlockStates::pack(unpacked);
    }

    reverse_delta::BlockStatesSnapshot BlockStatesView::packSnapshot(const time_t timestamp) const {
        return reverse_delta::BlockStatesSnapshot {pack(), timestamp};
    }

    size_t BlockStatesView::unpackedIndex(const uint8_t x, const uint8_t z) {
        return unpackedIndex(x, 0, z);
    }

    BlockStatesView BlockStatesView::create2DView(const uint16_t fill) {
        return {SECTION_2D_SIZE_BLOCKS, fill};
    }

    BlockStatesView BlockStatesView::create3DView(const uint16_t fill) {
        return BlockStatesView {SECTION_3D_SIZE_BLOCKS, fill};
    }

    BlockStatesView BlockStatesView::create2DView() {
        return BlockStatesView {SECTION_2D_SIZE_BLOCKS};
    }

    BlockStatesView BlockStatesView::create3DView() {
        return BlockStatesView {SECTION_3D_SIZE_BLOCKS};
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

    BlockStates::BlockStates(const Palette& palette, LongArray packedData, const size_t snapshotLength): packedData(std::move(packedData)) {
        this->snapshotLength = snapshotLength;
        this->palette = palette;
        this->bitsPerIndex = getBitsPerIndex(palette);
    }

    uint64_t BlockStates::getBitsPerIndex(const Palette& palette) {
        return std::max(static_cast<int>(std::ceil(log2(static_cast<double>(palette.size())))), 1);
    }

    UnpackedBlockStates BlockStates::unpack() const {
        UnpackedBlockStates unpacked(snapshotLength);
        const BitStorage bitStorage(bitsPerIndex, snapshotLength, packedData);

        size_t index = 0;
        for (size_t i = 0; i < snapshotLength; ++i) {
            const auto slice = bitStorage.get(i);
            unpacked[index] = palette[slice];
            ++index;
        }
        return unpacked;
    }

    BlockStates BlockStates::pack(const UnpackedBlockStates& sectionData) {
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

        return BlockStates(palette, bitStorage.data, snapshotLength);
    }

    BlockStatesView BlockStates::view() const {
        return BlockStatesView(unpack());
    }

}