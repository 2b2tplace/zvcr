#include <zvcr/common/data_storage.hpp>

#include <cassert>
#include <tuple>
#include <absl/container/flat_hash_map.h>

namespace zvcr {

    BitStorage::BitStorage(const size_t bits, const size_t size): bits(bits), size(size) {
        init();
    }

    auto BitStorage::init() -> void {
        assert(bits >= 1 && bits <= 32);

        valuesPerLong = 64 / bits;
        const size_t magicIndex = valuesPerLong - 1;
        std::tie(divideMul, divideAdd, divideShift) = MAGIC[magicIndex];
        packedLength = (size + valuesPerLong - 1) / valuesPerLong;
        if (packedLength > data.size())
            data.resize(packedLength);

        divideMul = static_cast<uint64_t>(static_cast<uint32_t>(divideMul));
        divideAdd = static_cast<uint64_t>(static_cast<uint32_t>(divideAdd));
        mask = (1ULL << bits) - 1;
    }

    auto BitStorage::cellIndex(const uint64_t index) const -> size_t {
        return (index * divideMul + divideAdd) >> 32 >> divideShift;
    }

    auto BitStorage::get(const size_t index) const -> uint64_t {
        assert(index < size && "Index out of bounds");

        const size_t cellIdx = cellIndex(index);
        const uint64_t cell = data[cellIdx];
        const size_t bitIndex = (index - cellIdx * valuesPerLong) * bits;
        return cell >> bitIndex & mask;
    }

    auto BitStorage::set(const size_t index, const uint64_t value) -> void {
        assert(index < size);
        assert(value <= mask);

        const size_t cellIdx = cellIndex(index);
        uint64_t& cell = data[cellIdx];
        const size_t bitIndex = (index - cellIdx * valuesPerLong) * bits;
        cell = cell & ~(mask << bitIndex) | (value & mask) << bitIndex;
    }

}