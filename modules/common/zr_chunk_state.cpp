#include <libzr/modules/common/zr_chunk_state.hpp>

std::ostream& operator<<(std::ostream& os, const ZrChunkStateType chunkStateType) {
    return os << ChunkStateTypeRegistry.at(chunkStateType);
}