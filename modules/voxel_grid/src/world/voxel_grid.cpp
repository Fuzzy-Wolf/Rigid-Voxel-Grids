#include <algorithm>
#include <string>

#include "core/error/error_macros.h"
#include "core/math/math_defs.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "core/math/vector3i.h"
#include "core/typedefs.h"
#include "../meta/MemoryManager.h"
#include "modules/voxel_grid/src/meta/VoxelGridGenerator.h"
#include "scene/3d/physics/physics_body_3d.h"

#include "../global.h"
#include "chunk.h"
#include "local_game_instance.h"
#include "voxel_grid.h"


VG::VoxelGrid::VoxelGrid(PhysicsBody3D *body) noexcept
    : physics_body_rid(body->get_rid())
    , body_data(body)
{
    if (unlikely(Engine::get_singleton()->is_editor_hint())) {
        return;
    }
    const auto ps = PhysicsServer3D::get_singleton();
    const auto game = LocalGame::get_singleton();

//  ps->body_set_mode(physics_body_rid, PhysicsServer3D::BODY_MODE_STATIC); // da ich kein RID mehr erzeuge: das hier nicht machen.
    ps->body_set_space(physics_body_rid, game->get_physics_space());
    ps->body_set_state(physics_body_rid, PhysicsServer3D::BODY_STATE_TRANSFORM, body_data->get_transform());
    //TODO: mesh mit PhysicsBody bewegen
    // ps->body_set_force_integration_callback(_physics_body, _body_moved);
}
VG::VoxelGrid::VoxelGrid(PhysicsBody3D *body, ushort radius) noexcept
    : VoxelGrid(body)
{
    generator = new PlanetGenerator(radius); // new_delete PlanetGenerator
}



VG::ChunkKoords VG::VoxelGrid::get_chunk_koords_from_local_voxel_pos(const Vector3 &vox_pos_in_grid) {
    return VG::ChunkKoords::from((vox_pos_in_grid / CHUNK_SIZE).floor());
}

// gibt die im-Chunk-Koordinaten des Voxels aus. [(0,0,0) bis (CHUNK_SIZE,CHUNK_SIZE,CHUNK_SIZE)]
VG::VoxelKoords VG::VoxelGrid::get_voxel_koords(const Vector3 vox_pos_in_grid, const ChunkKoords chunk_koords) {
    return VG::VoxelKoords::from((vox_pos_in_grid - (chunk_koords * CHUNK_SIZE)).floor());
}

/*
void add_update_data(const Vector3i chunk_koords, const VG::RADIUS radius, Chunk* chunk, const RID physics_body_rid) {
    const auto tp = ThreadPool::get_singleton();
    const Vector3 chunk_pos = Vector3(chunk_koords * VG::CHUNK_SIZE) * VG::VOXEL_SIZE;
    const auto cb = ChunkBuilder::get_singleton();

    tp->main_thread_storage.changing_chunks.emplace_back(VG::UpdateData{});
    auto& ud = tp->main_thread_storage.changing_chunks.back();
    if (radius <= VG::RADIUS::COLLISION_NEAR) {
        ud.r_col = new VG::RemoveColData{
            physics_body_rid,
            chunk->get_first_shape_rid(),
            chunk->shape_count };
        const auto shape_data = cb->generate_collision(_ , chunk_koords.x, chunk_koords.y, chunk_koords.z, chunk->voxels);
        ud.c_col = new VG::CreateColData{
            physics_body_rid,
            shape_data,
            chunk };
    }
    if (radius <= VG::RADIUS::DRAW_FULL) {
        ud.lod = new VG::LOD1MeshData{ chunk->get_mesh_rid() };
        ud.mesh = new VG::FullMeshData{ cb->generate_mesh_data(chunk->voxels),
            chunk->get_mesh_rid(),
            chunk->get_instance_rid(),
            Transform3D{ Basis(), chunk_pos } };
    }
}

void VG::VoxelGrid::remove_block(Vector3 global_col_pos, Vector3 normal) { // TODO: auch die richtung übergeben, so kann der "andere Chunk" über Nachbar ermittelt werden
    //TODO: Transformationen wie Rotation
    const auto game = LocalGameInstance::get_singleton();
    const auto player_pos = game->player->get_position();
    if (VOXEL_SIZE*3*3 < player_pos.distance_squared_to(global_col_pos)) {
        //TODO: Reichweitencheck sollte wahrscheinlich woanders stattfinden!
        return;
    }

    // finde voxel Position in Grid:
    Vector3 vox_pos_in_grid = (global_col_pos - get_position()) / VOXEL_SIZE;

    // gehe eine halbe Voxel-länge in Voxel hinein
    vox_pos_in_grid -= normal * VOXEL_SIZE * 0.5;
    
    // finde chunk Koordinaten in Grid:
    const Vector3i chunk_koords = get_chunk_koords_from_local_voxel_pos(vox_pos_in_grid);

    // Berechne voxel Koordinaten in Chunk:
    const Vector3i voxel_koords = get_voxel_koords(vox_pos_in_grid,chunk_koords);

    Chunk* chunk = game->get_chunk(chunk_koords, get_instance_id());
    ERR_FAIL_NULL_MSG(chunk,
        ("Chunk wurde nicht gefunden:\n    x: " + std::to_string(chunk_koords.x) + "\n    y: " + std::to_string(chunk_koords.y) + "\n    z: " + std::to_string(chunk_koords.z)).c_str());
    ERR_FAIL_NULL_MSG(chunk->voxels, "Chunk hat keine Voxel?!?");

    chunk->voxels->get(voxel_koords.x, voxel_koords.y, voxel_koords.z) = BLOCK_ID::NOTHING;

    
    const RADIUS radius = get_radius_of_chunk(
        chunk_koords,
        pos_to_koords_corner(player_pos));
    
    add_update_data(chunk_koords,radius,chunk,physics_body_rid);
    
    //TODO: if Randblock -> update neighbor_chunks
}


void VG::VoxelGrid::add_block(Vector3 global_col_pos, Vector3 normal) {
    //TODO: Transformationen wie Rotation
    const auto game = LocalGameInstance::get_singleton();
    const auto player_pos = game->player->get_position();
    if (VOXEL_SIZE*3*3 < player_pos.distance_squared_to(global_col_pos)) {
        //TODO: Reichweitencheck sollte wahrscheinlich woanders stattfinden!
        return;
    }
    // finde voxel Position in Grid
    Vector3 vox_pos_in_grid = (global_col_pos - get_position()) / VOXEL_SIZE;

    // gehe eine halbe Voxel-länge von Voxel weg
    vox_pos_in_grid += normal*VOXEL_SIZE*0.5;
    
    // finde chunk Koordinaten in Grid:
    const auto chunk_koords = get_chunk_koords_from_local_voxel_pos(vox_pos_in_grid);

    // Berechne voxel Koordinaten in Chunk:
    const auto voxel_koords = get_voxel_koords(vox_pos_in_grid,chunk_koords);

    Chunk* chunk = game->get_chunk(chunk_koords, get_instance_id());
    if (chunk) {
        chunk->voxels->get(voxel_koords.x, voxel_koords.y, voxel_koords.z) = BLOCK::DEBUG;
    } else {
        //TODO: Chunk generieren.
        return;
    }
    const RADIUS radius = get_radius_of_chunk(
        chunk_koords,
        pos_to_koords_corner(player_pos));
    
    add_update_data(chunk_koords,radius,chunk,physics_body_rid);
    
    //TODO: if Randblock -> update neighbor_chunks

}
*/

VG::ChunkMemoryUnit VG::VoxelGrid::summon_chunks(ChunkKoords koords) const {
    const auto mm = MemoryManager::get_singleton();
    ChunkMemoryUnit chunks = mm->load_chunks_from_disk(ChunkKey{koords,get_instance_id()});
    if (generator) {
        for (ChunkContent*& content : chunks) {
            if (!content) {
                generator->generate_chunk_without_voxels(ChunkKey{koords,get_instance_id()},data); //TODO: der key ist falsch!
            }
        }
    }
    return chunks;
}

VG::ChunkMemoryUnit VG::VoxelGrid::summon_chunks_with_voxels(ChunkKoords koords) const {
    const auto mm = MemoryManager::get_singleton();
    ChunkMemoryUnit chunks = mm->load_chunks_with_voxels_from_disk(ChunkKey{koords,get_instance_id()});
    if (generator) {
        for (ChunkContent *&content : chunks) {
            if (!content) {
                //TODO: Damit das auch mit mehr als einem Chunk in 'ChunkMemoryUnit' funktioniert, müssen die 'ChunkKey's erhaltbar sein
                // Chunk muss generiert werden
                content = generator->generate_chunk_with_voxels(ChunkKey{/* TODO: 'koords' ist falsch! */koords,get_instance_id()},data); //TODO: der key ist falsch!
            }
        }
    }
    return chunks;
}



VG::RADIUS VG::VoxelGrid::distance_player_chunk(const Vector3i &chunk_koords, const Vector3i &player_koords) {
    const auto dif = chunk_koords - player_koords;
    auto dist = std::max(dif.x, std::max(dif.y, dif.z));
    
    ERR_FAIL_COND_V_MSG(dist > COLLISION_NEAR,
        LOAD,
        ("Radius passt nicht! 'dist' = " +
        std::to_string(dist)).c_str());

    if (dist <= COLLISION_NEAR) {
        return COLLISION_NEAR;
    }
    if (dist <= DRAW_FULL) {
        return DRAW_FULL;
    }
    return LOAD;
}
VG::ChunkKoords VG::VoxelGrid::containing_chunk_koords(const Vector3 &pos) const {
    Vector3 local_pos = body_data->get_transform().xform_inv(pos);
    local_pos /= (CHUNK_SIZE * VOXEL_SIZE);
    return ChunkKoords::from(local_pos.floor());
}
VG::ChunkKoords VG::VoxelGrid::snap_to_chunk_koords(const Vector3 &pos) const {
    Vector3 local_pos = body_data->get_transform().xform_inv(pos);
    local_pos /= (CHUNK_SIZE * VOXEL_SIZE);
    local_pos += Vector3(0.5, 0.5, 0.5);
    return ChunkKoords::from(local_pos.floor());
}
VG::PGKoords VG::VoxelGrid::snap_to_PG_koords(const Vector3 &pos) const {
    Vector3 local_pos = body_data->get_transform().xform_inv(pos);
    local_pos /= (PG_SIZE * VOXEL_SIZE);
    local_pos += Vector3(0.5, 0.5, 0.5);
    return PGKoords::from(local_pos.floor() * (PG_SIZE * VOXEL_SIZE));
}
real_t VG::VoxelGrid::distance_to_BB(const Vector3 &pos) const {
    VKoords local_pos = VKoords::from(body_data->get_transform().xform_inv(pos));
    return data.extent.maxnorm_distance_to_border(local_pos);
}
bool VG::VoxelGrid::is_close(const Vector3 &pos) const {
    return distance_to_BB(pos) <= PG_SIZE;
}
const Transform3D VG::VoxelGrid::to_global_transform(ChunkKoords koords) const {
    Transform3D t = body_data->get_transform();
    t.origin = t.xform(koords * CHUNK_SIZE * VOXEL_SIZE);
    return t;
    //return body_data->get_transform().inverse_xform(Transform3D(Basis(), koords * CHUNK_SIZE * VOXEL_SIZE));
}
const Transform3D VG::VoxelGrid::to_global_transform(PGKoords koords) const {
    Transform3D t = body_data->get_transform();
    t.origin = t.xform(koords * VOXEL_SIZE);
    return t;
    //return body_data->get_transform().inverse_xform(Transform3D(Basis(), koords * VOXEL_SIZE));
}
