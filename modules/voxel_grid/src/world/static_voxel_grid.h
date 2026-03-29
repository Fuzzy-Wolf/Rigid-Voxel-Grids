#ifndef STATICVOXELGRID_H
#define STATICVOXELGRID_H

#include "scene/3d/physics/static_body_3d.h"

#include "../global.h"
#include "voxel_grid.h"


namespace VG {
class StaticVoxelGrid : public StaticBody3D, public VoxelGrid {
    GDCLASS(StaticVoxelGrid, StaticBody3D)

    
protected:
    static void _bind_methods();

public:
    StaticVoxelGrid() noexcept;
    StaticVoxelGrid(ushort size) noexcept;

    /*
    void remove_block(Vector3 global_col_pos, Vector3 normal) {VoxelGrid::remove_block(global_col_pos,normal);}
    void add_block(Vector3 global_col_pos, Vector3 normal) {VoxelGrid::add_block(global_col_pos,normal);}
    */
};
}; //namespace VVG

using VG::StaticVoxelGrid;

#endif // STATICVOXELGRID_H