#pragma once

#include <libzr/modules/zr_common.hpp>
#include <smmalloc.hpp>

typedef std::vector<uint16_t> Palette;
typedef std::vector<uint16_t> UnpackedBlockStates;

#define PREALLOC_POOL_SIZE 5368709120
#define PREALLOC_BUCKET_SIZE 52428800

extern size_t ALLOC_TEST_TOTAL;
extern sm_allocator PREALLOC;

class LongArray {
public:
    explicit LongArray(size_t size);
    ~LongArray();
    size_t size() const;
    void resize(size_t size);
    bool empty() const;
    uint64_t& operator[](size_t idx) const;
    uint64_t* data() const;
private:
    size_t _size;
    uint64_t *_data;
};

class ZrBlockStatesView {
public:
    explicit ZrBlockStatesView(const UnpackedBlockStates& unpacked);
    explicit ZrBlockStatesView(size_t snapshotLength);

    uint16_t getBlockState(uint8_t x, uint8_t y, uint8_t z) const;
    void setBlockState(uint8_t x, uint8_t y, uint8_t z, uint16_t blockStateId);
    uint16_t get(uint8_t x, uint8_t z) const;
    void set(uint8_t x, uint8_t z, uint16_t blockStateId);
    static size_t unpackedIndex(uint8_t x, uint8_t y, uint8_t z);
    static size_t unpackedIndex(uint8_t x, uint8_t z);

    UnpackedBlockStates unpacked;
};

class BitStorage {
public:
    BitStorage(size_t bits, size_t size, const LongArray& data);

    size_t cellIndex(uint64_t index) const;
    uint64_t get(size_t index) const;
    void set(size_t index, uint64_t value);

    LongArray data;
private:
    size_t bits;
    size_t size;
    uint64_t mask;
    size_t valuesPerLong;
    uint64_t divideMul;
    uint64_t divideAdd;
    int32_t divideShift;
};

class ZrBlockStates {
public:
    explicit ZrBlockStates(const Palette& palette, const LongArray& packedData, size_t snapshotLength);

    static uint64_t getBitsPerIndex(const Palette& palette);
    static ZrBlockStates pack(const UnpackedBlockStates& sectionData);
    UnpackedBlockStates unpack() const;
    ZrBlockStatesView view() const;

    size_t snapshotLength;
    LongArray packedData;
    Palette palette;
    uint64_t bitsPerIndex;
};

// get that sweet, sweet performance (average math fan vs lookup table enjoyer)
const std::tuple<int64_t, int64_t, int32_t> MAGIC[] = {
    {-1, -1, 0},
    {-2147483648, 0, 0},
    {1431655765, 1431655765, 0},
    {-2147483648, 0, 1},
    {858993459, 858993459, 0},
    {715827882, 715827882, 0},
    {613566756, 613566756, 0},
    {-2147483648, 0, 2},
    {477218588, 477218588, 0},
    {429496729, 429496729, 0},
    {390451572, 390451572, 0},
    {357913941, 357913941, 0},
    {330382099, 330382099, 0},
    {306783378, 306783378, 0},
    {286331153, 286331153, 0},
    {-2147483648, 0, 3},
    {252645135, 252645135, 0},
    {238609294, 238609294, 0},
    {226050910, 226050910, 0},
    {214748364, 214748364, 0},
    {204522252, 204522252, 0},
    {195225786, 195225786, 0},
    {186737708, 186737708, 0},
    {178956970, 178956970, 0},
    {171798691, 171798691, 0},
    {165191049, 165191049, 0},
    {159072862, 159072862, 0},
    {153391689, 153391689, 0},
    {148102320, 148102320, 0},
    {143165576, 143165576, 0},
    {138547332, 138547332, 0},
    {-2147483648, 0, 4},
    {130150524, 130150524, 0},
    {126322567, 126322567, 0},
    {122713351, 122713351, 0},
    {119304647, 119304647, 0},
    {116080197, 116080197, 0},
    {113025455, 113025455, 0},
    {110127366, 110127366, 0},
    {107374182, 107374182, 0},
    {104755299, 104755299, 0},
    {102261126, 102261126, 0},
    {99882960, 99882960, 0},
    {97612893, 97612893, 0},
    {95443717, 95443717, 0},
    {93368854, 93368854, 0},
    {91382282, 91382282, 0},
    {89478485, 89478485, 0},
    {87652393, 87652393, 0},
    {85899345, 85899345, 0},
    {84215045, 84215045, 0},
    {82595524, 82595524, 0},
    {81037118, 81037118, 0},
    {79536431, 79536431, 0},
    {78090314, 78090314, 0},
    {76695844, 76695844, 0},
    {75350303, 75350303, 0},
    {74051160, 74051160, 0},
    {72796055, 72796055, 0},
    {71582788, 71582788, 0},
    {70409299, 70409299, 0},
    {69273666, 69273666, 0},
    {68174084, 68174084, 0},
    {-2147483648, 0, 5}
};