#include "MemoryManager.h"
#include "modules/voxel_grid/src/world/chunk.h"
#include "modules/voxel_grid/src/world/local_game_instance.h"

VG::MemoryManager* const VG::MemoryManager::singleton = new VG::MemoryManager(); //new_delete MemoryManager

void VG::MemoryManager::queue_save_chunk_to_disk(const ChunkKey& key) const {
    // TODO:...
    // in einer liste sammeln (woanders dann alle zusammen speichern)
}

VG::ChunkMemoryUnit VG::MemoryManager::load_chunks_from_disk(const ChunkKey& key) const {
    // TODO: hier müssten jetzt Chunks geladen werden
    // sollte gesammelt passieren,

    // const auto game = LocalGame::get_singleton();
    // ChunkContent *content = game->push_chunk(key);

    return { nullptr };
}

VG::VoxelData *VG::MemoryManager::load_voxels_from_disk(const ChunkKey &key) const {
    VoxelData* voxels = new VoxelData(); // new_delete voxels;
    //TODO:...
    voxels->get(0, 0, 0) = 0xFF'FF'FF'FF; // TODO: entferne diesen workaround | Alle Chunks auf der Festplatte haben valide Voxel! (?)
    return voxels;
}

VG::ChunkMemoryUnit VG::MemoryManager::load_chunks_with_voxels_from_disk(const ChunkKey& key) const {
    ChunkMemoryUnit chunks = load_chunks_from_disk(key);
    
    for (ChunkContent *&content : chunks) {
        if (content) {
            // Chunk existiert auf der Festplatte:
            //TODO: ... lade Voxel daten...
            if (content->voxels == nullptr) {
                content->voxels = load_voxels_from_disk(key);
            }
        }
    }
    return chunks;
}
void VG::MemoryManager::queue_save_small_grid_to_disk(const VoxelGrid *const) const {
    //TODO: ...
}
