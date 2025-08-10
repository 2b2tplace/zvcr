#include <zvcr.hpp>

int main() {
    uint16_t protocolVersion{};
    const auto file = zvcr::readZVCRFile<zvcr::ZVCR3File>("r.-3.16.zvcr3", &protocolVersion);
    if (!file.has_value()) {
        std::cerr << "Could not read file, read error code = " << file.error().what() << '\n';
        return EXIT_FAILURE;
    }
    zvcr::writeZVCRFile(file.value(), "r.-3.16.zvcr3.bak", protocolVersion);
    return EXIT_SUCCESS;
}