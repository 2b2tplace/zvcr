#pragma once

#include <zvcr/io/file_location.hpp>
#include <zvcr/io/file_type.hpp>
#include <zvcr/io/compression.hpp>
#include <zvcr/io/serialize/context.hpp>
#include <zvcr/region/paletted_delta_data.hpp>
#include <zvcr/region/tile_entities.hpp>
#include <absl/container/flat_hash_map.h>
#include <fstream>

namespace zvcr {

    using PaletteTable = absl::flat_hash_map<Palette, size_t, PaletteHash>;

    class WriteHandle {
        PaletteTable blockPaletteTable{};
        PaletteTable biomePaletteTable{};

    public:
        int compressionLevel{ZSTD_COMPRESSION_LEVEL_DEFAULT};
        unsigned int compressionThreads{ZSTD_COMPRESSION_THREADS_DEFAULT};
        Context ctx{};
        std::vector<uint8_t> data;

        explicit WriteHandle(const Context &ctx,
                             const int compressionLevel = ZSTD_COMPRESSION_LEVEL_DEFAULT,
                             const unsigned int compressionThreads = ZSTD_COMPRESSION_THREADS_DEFAULT):
            compressionLevel(compressionLevel),
            compressionThreads(compressionThreads),
            ctx(ctx) {}

        explicit WriteHandle(const uint16_t protocolVersion,
                             const int compressionLevel = ZSTD_COMPRESSION_LEVEL_DEFAULT,
                             const unsigned int compressionThreads = ZSTD_COMPRESSION_THREADS_DEFAULT):
            WriteHandle(Context{.protocolVersion = protocolVersion}, compressionLevel, compressionThreads) {}

        [[nodiscard]]
        auto cloneParameters() const -> WriteHandle {
            return WriteHandle{ctx, compressionLevel, compressionThreads};
        }

        template<typename T>
        auto write(const T &value) -> void {
            data.resize(data.size() + sizeof(T));
            std::memcpy(data.data() + data.size() - sizeof(T), &value, sizeof(T));
        }

        template<typename T>
        auto writeArray(const T *array, const size_t length) -> void {
            const auto size = length * sizeof(T);
            data.resize(data.size() + size);
            std::memcpy(data.data() + data.size() - size, array, size);
        }

        template<typename Source>
        auto writeBytes(const Source &source) -> void {
            data.insert(data.end(), source.begin(), source.end());
        }

        auto writeByte(const uint8_t byte) -> void {
            data.push_back(byte);
        }

        template<size_t unpackedSize>
        auto serializePackedSnapshot(const PackedSnapshot<unpackedSize> &snapshot, PaletteTable &paletteTable) -> void {
            write<uint64_t>(snapshot.timestamp);

            const auto &anyData = snapshot.data.data;
            if (std::holds_alternative<uint16_t>(anyData)) { // 0 = single value palette
                write<uint8_t>(0);
                write<uint16_t>(std::get<uint16_t>(anyData));
                return;
            }
            write<uint8_t>(1); // 1 = section palette

            const auto &palettedData = std::get<PalettedData<unpackedSize>>(anyData);
            const auto &packedLongArray = palettedData.packedLongArray;
            const auto packedLength = packedLongArray.size();

            write<uint64_t>(packedLength);
            writeArray(packedLongArray.data(), packedLength);

            const auto paletteTableLength = paletteTable.size();
            const auto &palette = palettedData.palette;
            if (palette.direct()) {
                // don't store direct palettes, use uint32 max to encode direct palette
                write<uint32_t>(UINT32_MAX);
                return;
            }
            size_t paletteIndex = paletteTableLength;
            const auto existingPalette = paletteTable.find(palette);
            if (existingPalette != paletteTable.end()) {
                paletteIndex = existingPalette->second;
            } else {
                paletteTable.emplace(palette, paletteIndex);
            }
            write<uint32_t>(paletteIndex);
        }

        auto serializePaletteTable(const std::vector<Palette> &orderedPaletteTable) -> void;

        auto serializePaletteTable(const PaletteTable &paletteTable) -> void;

        template<size_t unpackedSize>
        auto serializePackedDeltaData(const PackedDeltaData<unpackedSize> &section3d, PaletteTable &paletteTable) -> void {
            write<uint64_t>(section3d.reverseDeltas.size());

            for (const auto &snapshot : section3d.reverseDeltas)
                serializePackedSnapshot(snapshot, paletteTable);
        }

        auto serializeSegmentState(const SegmentState &segmentState) -> void;

        auto serializeSegmentInfo(const SegmentInfo &segmentInfo) -> void;

        auto serializeTileEntities(const DeltaTileEntityData &tileEntities) -> void;

        auto serializeSegment(const Segment &segment) -> void;

        auto serializeOptSegment(const Segment *segmentOpt) -> void {
            if (!segmentOpt) {
                writeByte(0);
                return;
            }
            writeByte(1);
            serializeSegment(*segmentOpt);
        }

        auto serializeRegion(const Region &region) -> ZstdResult {
            auto regionContainer = cloneParameters();
            {
                auto handle = cloneParameters();
                handle.data.reserve(preallocate);

                for (size_t i = 0; i < SEGMENTS_PER_REGION; i++) {
                    handle.serializeOptSegment(region.segments[i].get());
                }
                regionContainer.serializePaletteTable(handle.blockPaletteTable);
                regionContainer.serializePaletteTable(handle.biomePaletteTable);
                regionContainer.writeBytes(handle.data);
            }
            std::vector<uint8_t> compressedRegionContainer;
            TRY(compressZstd(regionContainer.data, compressedRegionContainer, compressionLevel, compressionThreads));

            writeBytes(compressedRegionContainer);
            return {};
        }

        auto serializeFile(const File &file) -> ZstdResult;
    };

    using WriteResult = result::Result<size_t, std::string>;

    auto writeFile(const File &file, const fs::path &filepath,
                   int compressionLevel = ZSTD_COMPRESSION_LEVEL_DEFAULT,
                   unsigned int compressionThreads = ZSTD_COMPRESSION_THREADS_DEFAULT) -> WriteResult;

    auto writeFileAt(const File &file, const fs::path &parentDirectory, const RegionLocation &location,
                     int compressionLevel = ZSTD_COMPRESSION_LEVEL_DEFAULT,
                     unsigned int compressionThreads = ZSTD_COMPRESSION_THREADS_DEFAULT) -> WriteResult;
}
