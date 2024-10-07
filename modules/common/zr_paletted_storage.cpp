#include <libzr/modules/common/zr_paletted_storage.hpp>

ZrBlockStatesView::ZrBlockStatesView(const UnpackedBlockStates& unpacked) {
    this->unpacked = unpacked;
}

ZrBlockStatesView::ZrBlockStatesView(const size_t snapshotLength) {
    this->unpacked = UnpackedBlockStates(snapshotLength);
}

uint16_t ZrBlockStatesView::getBlockState(const uint8_t x, const uint8_t y, const uint8_t z) const {
    return unpacked[unpackedIndex(x, y, z)];
}

void ZrBlockStatesView::setBlockState(const uint8_t x, const uint8_t y, const uint8_t z, const uint16_t blockStateId) {
    unpacked[unpackedIndex(x, y, z)] = blockStateId;
}

size_t ZrBlockStatesView::unpackedIndex(const uint8_t x, const uint8_t y, const uint8_t z) {
    assert(x < CHUNK_SIDELENGTH);
    assert(y < CHUNK_SIDELENGTH);
    assert(z < CHUNK_SIDELENGTH);

    return static_cast<size_t>(y) * CHUNK_SIDELENGTH * CHUNK_SIDELENGTH
         + static_cast<size_t>(z) * CHUNK_SIDELENGTH
         + static_cast<size_t>(x);
}

uint16_t ZrBlockStatesView::get(const uint8_t x, const uint8_t z) const {
    return unpacked[unpackedIndex(x, z)];
}

void ZrBlockStatesView::set(const uint8_t x, const uint8_t z, const uint16_t blockStateId) {
    unpacked[unpackedIndex(x, z)] = blockStateId;
}

size_t ZrBlockStatesView::unpackedIndex(const uint8_t x, const uint8_t z) {
    return unpackedIndex(x, 0, z);
}

BitStorage::BitStorage(const size_t bits, const size_t size, const std::vector<uint64_t>& data = {}): bits(bits), size(size) {
    assert(bits >= 1 && bits <= 32);

    valuesPerLong = 64 / bits;
    const size_t magicIndex = valuesPerLong - 1;
    std::tie(divideMul, divideAdd, divideShift) = MAGIC[magicIndex];
    const size_t calculatedLength = (size + valuesPerLong - 1) / valuesPerLong;

    divideMul = static_cast<uint64_t>(static_cast<uint32_t>(divideMul));
    divideAdd = static_cast<uint64_t>(static_cast<uint32_t>(divideAdd));
    mask = (1ULL << bits) - 1;

    if (data.empty()) {
        this->data.resize(calculatedLength, 0);
        return;
    }
    assert(data.size() == calculatedLength);
    this->data = data;
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
    if (data.empty())
        return;

    assert(index < size);
    assert(value <= mask);

    const size_t cellIdx = cellIndex(index);
    uint64_t& cell = data[cellIdx];
    const size_t bitIndex = (index - cellIdx * valuesPerLong) * bits;
    cell = cell & ~(mask << bitIndex) | (value & mask) << bitIndex;
}

ZrBlockStates::ZrBlockStates(const Palette& palette, const LongArray& packedData, const size_t snapshotLength) {
    this->snapshotLength = snapshotLength;
    this->palette = palette;
    this->packedData = packedData;
    this->bitsPerIndex = getBitsPerIndex(palette);
}

uint64_t ZrBlockStates::getBitsPerIndex(const Palette& palette) {
    return std::max(static_cast<int>(std::ceil(log2(palette.size()))), 1);
}

UnpackedBlockStates ZrBlockStates::unpack() const {
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

ZrBlockStates ZrBlockStates::pack(const UnpackedBlockStates& sectionData) {
    std::set unique(sectionData.begin(), sectionData.end());
    std::vector palette(unique.begin(), unique.end());
    std::ranges::sort(palette);

    const auto snapshotLength = sectionData.size();
    BitStorage bitStorage(getBitsPerIndex(palette), snapshotLength);

    for (size_t i = 0; i < snapshotLength; ++i) {
        uint16_t state = sectionData[i];
        const auto stateIndexIt = std::ranges::find(palette, state);
        assert(stateIndexIt != palette.end());

        const size_t stateIndex = std::distance(palette.begin(), stateIndexIt);
        bitStorage.set(i, stateIndex);
    }
    return ZrBlockStates(palette, bitStorage.data, snapshotLength);
}

ZrBlockStatesView ZrBlockStates::view() const {
    return ZrBlockStatesView(unpack());
}