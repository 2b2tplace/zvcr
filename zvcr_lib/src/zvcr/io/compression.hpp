#pragma once

#include <vector>
#include <thread>
#include <cstdint>
#include <string>
#include <variant>
#include <result.hpp>

namespace zvcr {

    inline constexpr auto ZSTD_COMPRESSION_LEVEL_DEFAULT = 8;
    inline const auto ZSTD_COMPRESSION_THREADS_DEFAULT = std::thread::hardware_concurrency() / 2;

    using ZstdResult = result::Result<std::monostate, std::string>;

    auto compressZstd(const std::vector<uint8_t> &in, std::vector<uint8_t> &out,
                      int compressionLevel = ZSTD_COMPRESSION_LEVEL_DEFAULT,
                      unsigned int compressionThreads = ZSTD_COMPRESSION_THREADS_DEFAULT) -> ZstdResult;

    auto decompressZstd(const std::vector<uint8_t> &in, std::vector<uint8_t> &out) -> ZstdResult;

}