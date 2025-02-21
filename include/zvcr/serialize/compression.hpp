#pragma once

#include <cstdint>
#include <vector>

namespace zvcr::serialize::compression {

    std::vector<uint8_t> compressData(const std::vector<uint8_t>& inputData, int zstdCompressionLevel, int zstdCompressionThreads);
    std::vector<uint8_t> decompressData(const std::vector<uint8_t>& compressedData);

}
