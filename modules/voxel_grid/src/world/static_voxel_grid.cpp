#include "static_voxel_grid.h"
#include "local_game_instance.h"


VG::StaticVoxelGrid::StaticVoxelGrid() noexcept
    : VoxelGrid(this)
{}
VG::StaticVoxelGrid::StaticVoxelGrid(ushort size) noexcept
    : VoxelGrid(this,size)
{}

void VG::StaticVoxelGrid::_bind_methods() {
//    ClassDB::bind_method(D_METHOD("remove_block","position","normals"), &StaticVoxelGrid::remove_block);
//    ClassDB::bind_method(D_METHOD("add_block","position","normals"), &StaticVoxelGrid::add_block);
}
