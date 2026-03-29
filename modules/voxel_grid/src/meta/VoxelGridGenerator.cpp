#include "VoxelGridGenerator.h"
#include "../global.h"
#include "../world/local_game_instance.h"
#include "core/error/error_macros.h"
#include <string>


void VG::VoxelGridGenerator::generate_voxels(VG::ChunkContent *content, ChunkKoords koords, const GridData& grid) const {
    ERR_FAIL_NULL_MSG(content, "Chunk konnte scheinbar nicht erzeugt werden...");
    //TODO: 'is_valid' ist Teil des Workarounds (siehe TODO in 'load_voxels_from_disk').
    if (content->voxels == nullptr || !content->voxels->is_valid()) {
        content->voxels = new VoxelData(); // new_delete voxels
        fill_voxels_random(content->voxels, koords);
    }
}
VG::ChunkContent* VG::VoxelGridGenerator::generate_chunk_with_voxels(const ChunkKey& key, const GridData& grid) const {
    const auto game = LocalGame::get_singleton();
    ChunkContent *content = game->push_chunk(key);
    generate_voxels(content, key.koords, grid);
    return content;
}

void VG::VoxelGridGenerator::generate_chunk_without_voxels(const ChunkKey &key, const GridData& grid) const {
    const auto game = LocalGame::get_singleton();
    ChunkContent *content = game->push_chunk(key);
}

VG::ChunkContent *VG::PlanetGenerator::generate_chunk_with_voxels(const ChunkKey &key, const GridData &grid) const {
    if (key.koords.abs().max() <= target_radius) {
        return VoxelGridGenerator::generate_chunk_with_voxels(key,grid);
    }
    return nullptr;
}

void VG::VoxelGridGenerator::fill_voxels_random(VoxelData* voxels, ChunkKoords chunk_koords) const {
    for (ushort x = 0; x < CHUNK_SIZE; ++x) {
        for (ushort y = 0; y < CHUNK_SIZE; ++y) {
            for (ushort z = 0; z < CHUNK_SIZE; ++z) {
                const real_t noise_result = noise->get_noise_3d(
                    VOXEL_SIZE * ((x + CHUNK_SIZE * chunk_koords.x) + 33000),
                    VOXEL_SIZE * ((y + CHUNK_SIZE * chunk_koords.y) + 33000),
                    VOXEL_SIZE * ((z + CHUNK_SIZE * chunk_koords.z) + 33000)); // + 33000 = workaround, weil negative aufgrund eines Bugs nicht gehen
                if (noise_result < 0.02) {
                    voxels->get(x, y, z) = Voxel(BLOCK_ID::DEBUG, Voxel::SOL, false);
                } else {
                    voxels->get(x, y, z) = Voxel(BLOCK_ID::NOTHING, Voxel::VAC, true);
                }
            }
        }
    }
}
