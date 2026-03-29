#ifndef CHUNK_BUILDER_H
#define CHUNK_BUILDER_H

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <string>
#include <sys/types.h>

#include "core/error/error_macros.h"
#include "core/math/vector3.h"

#include "../global.h"
#include "../utils.h"
#include "local_game_instance.h"
#include "voxel_grid.h"

namespace VG {

class CollisionData {
    // TODO: kein Bitset verwenden und selber Implementieren. (Warum? siehe greedy_box_v2: ich kann solid mit VoxelHasShape vereinen und beide aufeinmal auswerten!)
    COLLISIONDATA(solid)
public:
    CollisionData(const VoxelKoords vk, const VoxelData *const voxels) {
        for (uint8_t x = 0; x < MAX_SHAPE_EXTENT; ++x) {
        for (uint8_t y = 0; y < MAX_SHAPE_EXTENT; ++y) {
        for (uint8_t z = 0; z < MAX_SHAPE_EXTENT; ++z) {
            set_solid(x, y, z, voxels->get(vk.x + x, vk.y + y, vk.z + z).is_solid());
            //TODO: andere Aggregate
        }}}
    }
};


class ChunkBuilder {
    static ChunkBuilder *const singleton;
public:
    static inline ChunkBuilder* get_singleton() { return singleton; }

/************************************************ Meshing ************************************************/
private:
    int _vertice_count = 0;
    
    PackedVector3Array _vertices;
    PackedVector2Array _uvs;
    PackedInt32Array _indices;
    PackedVector3Array _normals;

    // void greedy_face_xy(const Vector2i &begin, Vector2i &end, ushort z);
    // void greedy_face_yz(const Vector2i &begin, Vector2i &end, ushort x);
    // void greedy_face_xz(const Vector2i &begin, Vector2i &end, ushort y);
    void _create_face_xy(const Voxel block_id, const ushort x, const ushort y, ushort z_offset, bool invert_normals);
    void _create_face_yz(const Voxel block_id, const ushort x, const ushort y, ushort x_offset, bool invert_normals);
    void _create_face_xz(const Voxel block_id, const ushort x, const ushort y, ushort y_offset, bool invert_normals);
    void _add_polygons(bool invert_normals) noexcept;

public:
    Array generate_mesh_data(const VoxelData *data);

/************************************************ Collision ************************************************/
private:
    Bit8x8x8Array shape_indicator;

    void greedy_box(const Vector3i &begin, Vector3i &end, const VoxelData* data);
    VoxelKoords greedy_box_v2(const VoxelKoords begin, const CollisionData data);

public:
    BoxShapeContainer generate_collision_shapes(const CollisionData& data);
    
};
}; //namespace VG

using VG::ChunkBuilder;

#endif // CHUNK_BUILDER_H