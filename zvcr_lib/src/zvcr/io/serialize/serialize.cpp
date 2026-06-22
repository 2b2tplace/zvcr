#include <zvcr/io/serialize/serialize.hpp>

namespace zvcr {

    auto WriteHandle::serializePaletteTable(const std::vector<Palette> &orderedPaletteTable) -> void {
        write(static_cast<uint32_t>(orderedPaletteTable.size()));

        for (const auto &palette : orderedPaletteTable) {
            const auto paletteLength = palette.length();
            if (palette.direct() || paletteLength == 1) continue;  // don't store direct/single-value palettes

            write<uint16_t>(paletteLength);
            writeArray(palette.palette.data(), paletteLength);
        }
    }

    auto WriteHandle::serializePaletteTable(const PaletteTable &paletteTable) -> void {
        std::vector<Palette> orderedPaletteTable;
        orderedPaletteTable.resize(paletteTable.size());

        for (const auto &[palette, index] : paletteTable) {
            orderedPaletteTable[index] = palette;
        }
        serializePaletteTable(orderedPaletteTable);
    }

    auto WriteHandle::serializeSegmentState(const SegmentState &segmentState) -> void {
        writeByte(static_cast<uint8_t>(segmentState.type));
        write<uint64_t>(segmentState.timestamp);
    }

    auto WriteHandle::serializeSegmentInfo(const SegmentInfo &segmentInfo) -> void {
        write<uint64_t>(segmentInfo.segmentStates.size());

        for (const SegmentState &state : segmentInfo.segmentStates)
            serializeSegmentState(state);
    }

    auto WriteHandle::serializeTileEntities(const DeltaTileEntityData &tileEntities) -> void {
        write<uint64_t>(tileEntities.reverseDeltas.size());
        for (const auto &[timestamp, deltas] : tileEntities.reverseDeltas) {
            write<uint64_t>(timestamp);
            write<uint64_t>(deltas.size());

            std::vector<TileEntityPosition> sortedPositions;
            sortedPositions.reserve(deltas.size());
            for (const auto &pos : deltas | std::views::keys) {
                sortedPositions.push_back(pos);
            }
            std::ranges::sort(sortedPositions, [](const auto a, const auto b) {
                return a.packedPosition() < b.packedPosition();
            });
            for (const auto &pos : sortedPositions) {
                const auto &delta = deltas.at(pos);
                write<uint32_t>(pos.packedPosition());
                if (!std::holds_alternative<TileEntity>(delta)) {
                    write<uint8_t>(0);
                    continue;
                }
                write<uint8_t>(1);
                const auto &tileEntity = std::get<TileEntity>(delta);
                write<uint32_t>(tileEntity.type);
                write<uint64_t>(tileEntity.nbt.size());
                writeBytes(tileEntity.nbt);
            }
        }
    }

    auto WriteHandle::serializeFile(const File &file) -> ZstdResult {
        ctx.initialize(ZVCR3D_LATEST_VERSION);
        ctx.initializeSectionCount(file.dimensionType);

        writeBytes(filePrefix);
        writeByte(static_cast<uint8_t>(ZVCR3D_LATEST_VERSION));
        writeByte(static_cast<uint8_t>(file.dimensionType));
        write<uint16_t>(ctx.protocolVersion);
        return serializeRegion(file.region);
    }

    auto writeFile(const File &file, const fs::path &filepath, const int compressionLevel, const uint compressionThreads) -> WriteResult {
        WriteHandle handle{file.protocolVersion, compressionLevel, compressionThreads};
        if (const auto result = handle.serializeFile(file); !result)
            return ERR(result.error());

        std::ofstream fileStream(filepath, std::ios::out | std::ios::binary);
        if (!fileStream)
            return ERR("Failed to open file for writing: " + filepath.string());

        fileStream.write(reinterpret_cast<const char*>(handle.data.data()), static_cast<std::streamsize>(handle.data.size()));
        return handle.data.size();
    }

    auto writeFileAt(const File &file, const fs::path &parentDirectory, const RegionLocation &location,
        const int compressionLevel, const uint compressionThreads) -> WriteResult {
        fs::create_directories(location.directory(parentDirectory));
        return writeFile(file, location.filePath(parentDirectory), compressionLevel, compressionThreads);
    }

    auto WriteHandle::serializeSegment(const Segment &segment) -> void {
        for (size_t i = 0; i < ctx.sectionCount; i++) {
            const auto &section = segment.blockSections.sections.at(i);
            serializePackedDeltaData(section, blockPaletteTable);
        }
        for (size_t i = 0; i < ctx.sectionCount; i++) {
            const auto &section = segment.biomeSections.sections.at(i);
            serializePackedDeltaData(section, biomePaletteTable);
        }
        serializeSegmentInfo(segment.info);
        serializeTileEntities(segment.tileEntities);
    }

}