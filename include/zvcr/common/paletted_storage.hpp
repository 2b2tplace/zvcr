#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include <tuple>
#include <vector>
#include <zvcr/common/reverse_delta.hpp>

namespace zvcr::common::paletted_storage {

    using LongArray = std::vector<uint64_t>;
    using Palette = std::vector<uint16_t>;
    using UnpackedBlockStates = std::vector<uint16_t>;

    class BlockStatesView;

    class BlockStates {
    public:
        explicit BlockStates(const Palette& palette, LongArray packedData, size_t snapshotLength);

        [[nodiscard]]
        static uint64_t getBitsPerIndex(const Palette& palette);

        [[nodiscard]]
        static BlockStates pack(const UnpackedBlockStates& sectionData);

        [[nodiscard]]
        UnpackedBlockStates unpack() const;

        [[nodiscard]]
        BlockStatesView view() const;

        size_t snapshotLength;
        LongArray packedData;
        Palette palette;
        uint64_t bitsPerIndex;
    };

    class BlockStatesView {
    public:
        UnpackedBlockStates unpacked;

        explicit BlockStatesView(size_t snapshotLength);

        BlockStatesView(size_t snapshotLength, uint16_t fill);

        explicit BlockStatesView(const UnpackedBlockStates& unpacked);

        [[nodiscard]]
        uint16_t getBlockState(uint8_t x, uint8_t y, uint8_t z) const;

        void setBlockState(uint8_t x, uint8_t y, uint8_t z, uint16_t blockStateId);

        [[nodiscard]]
        uint16_t get(uint8_t x, uint8_t z) const;

        void set(uint8_t x, uint8_t z, uint16_t blockStateId);

        [[nodiscard]]
        BlockStates pack() const;

        [[nodiscard]]
        reverse_delta::BlockStatesSnapshot packSnapshot(time_t timestamp) const;

        [[nodiscard]]
        static size_t unpackedIndex(uint8_t x, uint8_t y, uint8_t z);

        [[nodiscard]]
        static size_t unpackedIndex(uint8_t x, uint8_t z);

        [[nodiscard]]
        static BlockStatesView create2DView(uint16_t fill);

        [[nodiscard]]
        static BlockStatesView create3DView(uint16_t fill);

        [[nodiscard]]
        static BlockStatesView create2DView();

        [[nodiscard]]
        static BlockStatesView create3DView();
    };

    class BitStorage {
    public:
        LongArray data;

        BitStorage(size_t bits, size_t size, const LongArray& data);

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
