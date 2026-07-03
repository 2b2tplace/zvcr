#include <zvcr/io/compression.hpp>
#include <zstd.h>

namespace zvcr {

    auto compressZstd(const std::vector<uint8_t> &in, std::vector<uint8_t> &out,
                      const int compressionLevel,
                      const unsigned int compressionThreads) -> ZstdResult {
        ZSTD_CCtx *cctx = ZSTD_createCCtx();
        if (!cctx)
            return ERR("Failed to create ZSTD_CCtx");

        auto result = ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, compressionLevel);
        if (ZSTD_isError(result)) {
            ZSTD_freeCCtx(cctx);
            return ERR("Failed to set ZSTD_CCtx compressionLevel parameter: " + std::string(ZSTD_getErrorName(result)));
        }
        if (compressionThreads > 0) {
            result = ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, static_cast<int>(compressionThreads));
            if (ZSTD_isError(result)) {
                ZSTD_freeCCtx(cctx);
                return ERR("Failed to set ZSTD_CCtx compressionThreads parameter: " + std::string(ZSTD_getErrorName(result)));
            }
        }
        const auto maxCompressedSize = ZSTD_compressBound(in.size());
        out.resize(maxCompressedSize);

        const auto compressedSize = ZSTD_compress2(cctx, out.data(), out.size(), in.data(), in.size());
        ZSTD_freeCCtx(cctx);

        if (ZSTD_isError(compressedSize)) {
            out.clear();
            return ERR("Failed to compress data: " + std::string(ZSTD_getErrorName(result)));
        }
        out.resize(compressedSize);
        return {};
    }

    auto decompressZstd(const std::vector<uint8_t> &in, std::vector<uint8_t> &out) -> ZstdResult {
        ZSTD_DCtx *dctx = ZSTD_createDCtx();
        if (!dctx)
            return ERR("Failed to create ZSTD_DCtx");

        const auto decompressedSize = ZSTD_getFrameContentSize(in.data(), in.size());

        if (decompressedSize == ZSTD_CONTENTSIZE_ERROR) {
            ZSTD_freeDCtx(dctx);
            return ERR("Input is not a valid ZSTD frame");
        }
        if (decompressedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
            ZSTD_freeDCtx(dctx);
            return ERR("Original size is not stored in the ZSTD frame");
        }
        out.resize(decompressedSize);

        const auto result = ZSTD_decompressDCtx(dctx, out.data(), out.size(), in.data(), in.size());
        ZSTD_freeDCtx(dctx);

        if (ZSTD_isError(result)) {
            out.clear();
            return ERR("Failed to decompress data: " + std::string(ZSTD_getErrorName(result)));
        }
        if (result != decompressedSize) {
            out.clear();
            return ERR("Decompressed size mismatch");
        }
        return {};
    }

}