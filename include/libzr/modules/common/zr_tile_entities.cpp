#include <libzr/modules/common/zr_tile_entities.hpp>

std::ostream& operator<<(std::ostream& os, const ZrTileEntityType tileEntityType) {
    return os << TileEntityTypeRegistry.at(tileEntityType);
}