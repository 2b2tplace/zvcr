#include <stdexcept>
#include <zvcr/serialize/compression.hpp>
#include <zstd.h>

namespace zvcr::serialize {

    std::vector<uint8_t> compressData(const std::vector<uint8_t>& inputData, const int zstdCompressionLevel, const int zstdCompressionThreads) {
        thread_local ZSTD_CCtx* cctx = ZSTD_createCCtx();
        ZSTD_CCtx_reset(cctx, ZSTD_reset_session_only);
        ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, zstdCompressionThreads);

        const size_t compressedSize = ZSTD_compressBound(inputData.size());
        std::vector<uint8_t> compressedData(compressedSize);

        const size_t actualCompressedSize = ZSTD_compressCCtx(cctx, compressedData.data(), compressedSize,
            inputData.data(), inputData.size(), zstdCompressionLevel);

        if (ZSTD_isError(actualCompressedSize)) {
            // ZSTD_freeCCtx(cctx); // not needed when using thread_local zstd context
            throw std::runtime_error("ZSTD compression failed: " + std::string(ZSTD_getErrorName(actualCompressedSize)));
        }
        // ZSTD_freeCCtx(cctx); // not needed when using thread_local zstd context
        compressedData.resize(actualCompressedSize);
        return compressedData;
    }

    std::vector<uint8_t> decompressData(const std::vector<uint8_t>& compressedData) {
        std::vector<uint8_t> decompressedData;
        thread_local ZSTD_DCtx* dctx = ZSTD_createDCtx();
        ZSTD_DCtx_reset(dctx, ZSTD_reset_session_only);
        const auto decompressedSize = ZSTD_getFrameContentSize(compressedData.data(), compressedData.size());

        decompressedData.resize(decompressedSize);
        const size_t actualDecompressedSize = ZSTD_decompressDCtx(dctx, decompressedData.data(), decompressedSize,
            compressedData.data(), compressedData.size());

        if (ZSTD_isError(actualDecompressedSize)) {
            // ZSTD_freeDCtx(dctx); // not needed when using thread_local zstd context
            throw std::runtime_error("ZSTD decompression failed: " + std::string(ZSTD_getErrorName(actualDecompressedSize)));
        }
        // ZSTD_freeDCtx(dctx); // not needed when using thread_local zstd context
        return decompressedData;
    }

}