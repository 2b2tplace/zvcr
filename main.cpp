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
    const auto t0 = std::chrono::high_resolution_clock::now();
    auto newFile = zvcr::readFileAt("test_files", location);
    if (!newFile.has_value()) {
        std::cerr << "Could not read file, read error = " << newFile.error().what() << '\n';
        return EXIT_FAILURE;
    }
    const auto t1 = std::chrono::high_resolution_clock::now();
    const auto writeResult = zvcr::writeFile(newFile.value(), location.directory("test_files") / (location.fileName() + ".bak"));
    if (!writeResult.has_value()) {
        std::cerr << "Could not write file, write error = " << writeResult.error() << '\n';
        return EXIT_FAILURE;
    }
    const auto t2 = std::chrono::high_resolution_clock::now();

    std::cout << "Read took " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0) << '\n';
    std::cout << "Write took " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1) << '\n';
    std::cout << "Wrote " << writeResult.value() << " bytes\n";
}