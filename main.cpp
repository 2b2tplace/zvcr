#include <zvcr.hpp>

int main() {
    uint16_t protocolVersion{};
    const auto t1 = std::chrono::high_resolution_clock::now();
    const auto file = zvcr::readZVCRFile<zvcr::ZVCR3File>("r.-3.16.zvcr3", &protocolVersion);
    if (!file.has_value()) {
        std::cerr << "Could not read file, read error = " << file.error().what() << '\n';
        return EXIT_FAILURE;
    }
    const auto t2 = std::chrono::high_resolution_clock::now();
    const auto writeResult = zvcr::writeZVCRFile(file.value(), "r.-3.16.zvcr3.bak", protocolVersion);
    if (!writeResult.has_value()) {
        std::cerr << "Could not write file, write error = " << writeResult.error() << '\n';
        return EXIT_FAILURE;
    }
    const auto t3 = std::chrono::high_resolution_clock::now();

    std::cout << "Read took " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1) << ", write took " << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2) << '\n';
    std::cout << "Wrote " << writeResult.value() << " bytes\n";
    return EXIT_SUCCESS;
}