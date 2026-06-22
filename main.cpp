#include <zvcr.hpp>
#include <iostream>
#include <chrono>
#include <array>
#include <random>

template<size_t sectionSize>
void testPalettePacking() {
    static zvcr::UnpackedData<sectionSize> buffer{};
    if (buffer.empty()) {
        for (size_t i = 0; i < buffer.size(); i++) {
            buffer[i] = i;
        }
    }
    for (uint16_t bound = 0; bound < sectionSize; bound++) {
        const auto packed = zvcr::PackedData<sectionSize>::pack(buffer);
        const auto unpacked = packed.unpack();
        assert(buffer == unpacked);
    }
}

int main() {
    testPalettePacking<zvcr::SECTION_SIZE_BLOCKS>();
    testPalettePacking<zvcr::SECTION_SIZE_BIOMES>();

    static constexpr auto location = zvcr::RegionLocation{
        .rx = -1,
        .rz = -1,
        .dimensionType = zvcr::DimensionType::OVERWORLD
    };
    const auto t1 = std::chrono::high_resolution_clock::now();
    auto oldFile = zvcr::readFileAtLegacy("legacy_test/legacy", location);
    if (!oldFile.has_value()) {
        std::cerr << "Could not read file, read error = " << oldFile.error().what() << '\n';
        return EXIT_FAILURE;
    }
    const auto t2 = std::chrono::high_resolution_clock::now();

    // Re-pack all data using the new bits per index calculation
    for (auto &segment : oldFile->region.segments) {
        if (!segment) continue;
        for (auto &section : segment->blockSections.sections) {
            for (auto &snapshot : section.reverseDeltas) {
                const auto unpacked = snapshot.data.unpack();
                snapshot.data = zvcr::PackedData<zvcr::SECTION_SIZE_BLOCKS>::pack(unpacked);
                assert(snapshot.data.unpack() == unpacked);
            }
        }
        for (auto &section : segment->biomeSections.sections) {
            for (auto &snapshot : section.reverseDeltas) {
                const auto unpacked = snapshot.data.unpack();
                snapshot.data = zvcr::PackedData<zvcr::SECTION_SIZE_BIOMES>::pack(unpacked);
                assert(snapshot.data.unpack() == unpacked);
            }
        }
    }
    const auto t3 = std::chrono::high_resolution_clock::now();
    {
        const auto writeResult = zvcr::writeFileAt(oldFile.value(), "legacy_test/updated", location);
        if (!writeResult.has_value()) {
            std::cerr << "Could not write file, write error = " << writeResult.error() << '\n';
            return EXIT_FAILURE;
        }
        std::cout << "Wrote " << writeResult.value() << " bytes\n";
    }
    const auto t4 = std::chrono::high_resolution_clock::now();

    std::cout << "Read (old) took " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    std::cout << ", re-pack (old -> new) took " << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2);
    std::cout << ", write (new) took " << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3) << '\n';

    const auto t5 = std::chrono::high_resolution_clock::now();
    auto newFile = zvcr::readFileAt(
        "legacy_test/updated",
        location
    );
    if (!newFile.has_value()) {
        std::cerr << "Could not read file, read error = " << oldFile.error().what() << '\n';
        return EXIT_FAILURE;
    }
    const auto t6 = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < zvcr::SEGMENTS_PER_REGION; i++) {
        const auto &oldSegment = oldFile->region.segments.at(i);
        const auto &newSegment = newFile->region.segments.at(i);

        if (oldSegment == nullptr != (newSegment == nullptr)) {
            std::cerr << "Segment optional mismatch: " << (oldSegment == nullptr) << " != " << (newSegment == nullptr) << '\n';
            return EXIT_FAILURE;
        }
        if (!oldSegment) continue;

        if (oldSegment->info.segmentStates != newSegment->info.segmentStates) {
            std::cerr << "Segment info mismatch\n";
            return EXIT_FAILURE;
        }
        if (oldSegment->tileEntities.reverseDeltas != newSegment->tileEntities.reverseDeltas) {
            std::cerr << "Tile entity history mismatch\n";
            return EXIT_FAILURE;
        }
        if (oldSegment->sectionCount != newSegment->sectionCount) {
            std::cerr << "Section count mismatch: " << oldSegment->sectionCount << " != " << newSegment->sectionCount << '\n';
            return EXIT_FAILURE;
        }
        for (size_t j = 0; j < oldSegment->sectionCount; j++) {
            const auto &oldSection = oldSegment->blockSections.sections.at(j);
            const auto &newSection = newSegment->blockSections.sections.at(j);

            if (oldSection.reverseDeltas.size() != newSection.reverseDeltas.size()) {
                std::cerr << "Section reverse delta count mismatch: " << oldSection.reverseDeltas.size() << " != "
                          << newSection.reverseDeltas.size() << '\n';
                return EXIT_FAILURE;
            }
            for (size_t k = 0; k < oldSection.reverseDeltas.size(); k++) {
                const auto &oldDelta = oldSection.reverseDeltas.at(k);
                const auto &newDelta = newSection.reverseDeltas.at(k);

                if (oldDelta.timestamp != newDelta.timestamp) {
                    std::cerr << "Delta timestamp mismatch: " << oldDelta.timestamp << " != " << newDelta.timestamp << '\n';
                    return EXIT_FAILURE;
                }
                const auto &oldUnpacked = oldDelta.data.unpack();
                const auto &newUnpacked = newDelta.data.unpack();
                if (oldUnpacked != newUnpacked) {
                    std::cerr << "Data mismatch\n";
                    return EXIT_FAILURE;
                }
            }
        }
    }

    const auto t7 = std::chrono::high_resolution_clock::now();
    {
        const auto writeResult = zvcr::writeFile(oldFile.value(),
            std::filesystem::path(location.directory("legacy_test/updated"))
            / (location.fileName() + ".bak")
        );
        if (!writeResult.has_value()) {
            std::cerr << "Could not write file, write error = " << writeResult.error() << '\n';
            return EXIT_FAILURE;
        }
        std::cout << "Wrote " << writeResult.value() << " bytes (rewritten)\n";
    }
    const auto t8 = std::chrono::high_resolution_clock::now();

    std::cout << "Read (new) took " << std::chrono::duration_cast<std::chrono::milliseconds>(t6 - t5) << '\n';
    std::cout << ", compare to old took " << std::chrono::duration_cast<std::chrono::milliseconds>(t7 - t6) << '\n';
    std::cout << ", write (new, rewritten) took " << std::chrono::duration_cast<std::chrono::milliseconds>(t8 - t7) << '\n';
}