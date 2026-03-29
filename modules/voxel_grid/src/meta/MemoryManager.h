#ifndef THREAD_SAVE_H
#define THREAD_SAVE_H


#include <array>

#include "../world/chunk.h"
#include "../global.h"
#include "modules/voxel_grid/src/world/VoxelData.h"

namespace VG {

using ChunkMemoryUnit = std::array<VG::ChunkContent *, CHUNK_MEMORY_UNIT_SIZE>;

class VoxelGrid;

class MemoryManager {
    static MemoryManager * const singleton;

public:
    static MemoryManager *get_singleton() { return singleton; }
    MemoryManager() noexcept = default;
    
    // wenn chunk bereits geladen werden nur die Voxel geladen. Sind Voxel ebenfalls geladen, passiert nichts.
    ChunkMemoryUnit load_chunks_with_voxels_from_disk(const ChunkKey&) const;
    VG::ChunkMemoryUnit load_chunks_from_disk(const ChunkKey &) const;
    VoxelData *load_voxels_from_disk(const ChunkKey &) const;
    void queue_save_chunk_to_disk(const ChunkKey&) const;
    void queue_save_small_grid_to_disk(const VoxelGrid *const) const;
};





}; //namespace VG

#endif // THREAD_SAVE_H