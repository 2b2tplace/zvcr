#include <libzr/modules/common/zr_dimension.hpp>

std::ostream& operator<<(std::ostream& os, const ZrDimensionType dimensionType) {
    return os << DimensionTypeRegistry.at(dimensionType);
}