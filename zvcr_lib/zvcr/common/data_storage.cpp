#include <zvcr/common/data_storage.hpp>

#include <cassert>
#include <tuple>
#include <absl/container/flat_hash_map.h>

namespace zvcr::paletted_storage {

    BitStorage::BitStorage(const size_t bits, const size_t size, const LongArray& data): data(data), bits(bits), // NOLINT(*-pro-type-member-init)
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

}