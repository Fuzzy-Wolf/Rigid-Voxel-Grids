#include "core/error/error_macros.h"
#include "core/math/basis.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "core/math/vector3i.h"
#include "modules/voxel_grid/src/global.h"
#include "modules/voxel_grid/src/meta/MemoryManager.h"
#include "modules/voxel_grid/src/utils.h"
#include "modules/voxel_grid/src/world/chunk.h"
#include "servers/rendering_server.h"

#include "../world/local_game_instance.h"
#include "../world/voxel_grid.h"
#include "../world/chunk_builder.h"
#include "ServerIntervention.h"

#include "tasks.h"

void VG::ServerIntervention::CreateMesh::add_to_commit() {
    const auto game = LocalGame::get_singleton();
    const auto rs = RenderingServer::get_singleton();
    if (!mesh_data.is_empty()) {
        rs->mesh_clear(mesh);
        rs->mesh_add_surface_from_arrays(mesh, RenderingServer::PRIMITIVE_TRIANGLES, mesh_data);
        rs->mesh_surface_set_material(mesh, 0, game->get_material());
        rs->instance_set_visible(instance, true);
        //TODO: das hier muss wo anders hin...! (wegen: physics integration)
        rs->instance_set_transform(instance, voxel_grid->to_global_transform(chunk_kooooooords));
    }
}
void VG::ServerIntervention::CreateMesh::commit() {
    //TODO: gibt es hier etwas zu tun?
}

void VG::ServerIntervention::CreateLOD1::add_to_commit() {
//    const auto game = LocalGameInstance::get_singleton();
    const auto rs = RenderingServer::get_singleton();
    rs->mesh_clear(mesh);
    // ERR_PRINT("Erzeuge LOD...(nicht wirklich...)");
    //TODO: LOD1 implementieren!
}
void VG::ServerIntervention::CreateLOD1::commit() {
    //TODO: ...
}



void VG::ServerIntervention::RemoveCollision::add_to_commit() {
    const auto begin_time = std::chrono::steady_clock::now();        

    const auto ps = PhysicsServer3D::get_singleton();
    const auto game = LocalGame::get_singleton();
    
    ERR_FAIL_COND_EDMSG(shape_count == 0,"Shape-count war 0!");
    const int body_shape_count = ps->body_get_shape_count(body);
    ERR_FAIL_COND_EDMSG(body_shape_count < shape_count,
            ("FUCK! Es stimmt etwas mit den Shape-counts nicht!\n'body_shape_count': " + std::to_string(body_shape_count) + "\n'shape_count': " + std::to_string(shape_count)).c_str());

    int first_shape_index = -1;
    
    for (int i = 0; i < body_shape_count; ++i) {
        if (first_shape == ps->body_get_shape(body, i)) {
            first_shape_index = i;
            for (int j = i; j < i + shape_count; ++j) {
                // Speicher alle frei werdenden RIDs (Recycling):
                game->unused_box_shapes.emplace_back(ps->body_get_shape(body, j));
            }
            break;
        }
    }
    ERR_FAIL_COND_EDMSG(first_shape_index == -1,"fuck! FirstShape wurde nicht gefunden!");
    ps->body_remove_n_shapes(body, first_shape_index, shape_count);

    const auto deltatime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin_time).count();
    // ERR_PRINT_ED(("RemoveColData::add_to_commit() - Laufzeit: " + std::to_string(deltatime)).c_str());
}
void VG::ServerIntervention::RemoveCollision::commit() {
    //TODO: hier muss "_shapes_changed()" aufgerufen werden. UND natürlich diesen Aufruf aus "body_remove_n_shapes" entfernen.
}



void VG::ServerIntervention::CreateCollision::add_to_commit() {
    const auto begin_time = std::chrono::steady_clock::now();

    Transform3D transform = Transform3D(Basis(), pg_koords);
    RID first_box = fetch_new_box_and_add_to_body(0, transform);
    
    for (size_t i = 1 ; i < shapes.size(); ++i) {
        //TODO: hier muss noch mehr gemacht werden (??)
        fetch_new_box_and_add_to_body(i, transform);
    }

    voxel_grid->get_physics().add_shape_holder(ShapeHolder{ pg_koords, first_box, static_cast<uint8_t>(shapes.size()) });




    /******************** DEBUGGING ********************/
    const auto game = LocalGame::get_singleton();
    RenderingServer *const RenderingServer = RenderingServer::get_singleton();
    
	// RID instance = RenderingServer->instance_create();
	// RenderingServer->instance_set_scenario(instance, game->get_draw_scenario());
    // RID mesh = RenderingServer->get_test_cube();
    // RenderingServer->instance_set_base(instance, mesh);
    // // transform.origin += VoxelKoords(PG_SIZE) / 2.;
    // Transform3D t = voxel_grid->to_global_transform(pg_koords);
    // t.scale_basis(VoxelKoords{PG_SIZE} / 2.);
    // RenderingServer->instance_set_transform(instance, t);
    // // ERR_PRINT(("origin: " + to_str(t.origin)).c_str());



    const auto deltatime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin_time).count();
    // ERR_PRINT_ED(("CreateColData::add_to_commit() - Laufzeit: " + std::to_string(deltatime) +
    //     "\n für " + std::to_string(shapes.size()) +
    //     " shapes.\n" + "Transform.origin: " + to_str(transform.origin) +
    //     "\n player_pos: " + to_str(game->get_player_pos()) +
    //     "\n ShapeHolder-koords: " + pg_koords.to_str()
    //     ).c_str());
}

void VG::ServerIntervention::CreateCollision::commit() {
    auto game = LocalGame::get_singleton();
    for (const VoxelGrid *const vg : game->get_large_grids()) {
        auto& deleted_shape_holders = vg->get_physics().get_deleted_shape_holders();
        if (MAX_DELETED_SHAPE_HOLDER_COUNT < deleted_shape_holders.size()) {
            ERR_PRINT("Lösche deleted_shape_holders!");
            for (ShapeHolder& sh : deleted_shape_holders) {
                RemoveCollision{vg->get_body_rid(),sh.get_first_shape_rid(),sh.get_shape_count()}.add_to_commit();
            }
            RemoveCollision::commit();
        }
        deleted_shape_holders.clear();
        vg->get_physics().physics_update.release();
    }
    // TODO: hier muss "_shapes_changed()" aufgerufen werden. UND es muss auch "body_prepare_adding_shape" implementiert werden.
}

RID VG::ServerIntervention::CreateCollision::fetch_new_box_and_add_to_body(const size_t index, const Transform3D &transform) const {
    const auto ps = PhysicsServer3D::get_singleton();
    const auto game = LocalGame::get_singleton();
    RID box;
    if (game->unused_box_shapes.empty()) {
        box = ps->box_shape_create();
    } else {
        box = game->unused_box_shapes.back();
        game->unused_box_shapes.pop_back();
    }
    ps->shape_set_data(box, shapes[index].size_times_2 / 2.);
    
    // TODO: body_add_n_shapes implementieren und benutzen!!!!!!!!!!!
    ps->body_add_shape(
        voxel_grid->get_body_rid(),
        box,
        transform.translated(shapes[index].koords_times_2 / 2.));

    return box;
}




void VG::ServerIntervention::LoadChunk::add_to_commit() {
//    const auto game = LocalGameInstance::get_singleton();
//    const auto rs = RenderingServer::get_singleton();
    // TODO: muss in Callback passieren und in "Physics3DServer.body_set_force_integration_callback(body, self, "_body_moved")" übergeben werden!!
    //rs->instance_set_transform(mesh_instance_rid, transform);
}
void VG::ServerIntervention::LoadChunk::commit() {}



void VG::ServerIntervention::UnLoadChunk::add_to_commit() {
    const auto rs = RenderingServer::get_singleton();
//    const auto ps = PhysicsServer3D::get_singleton();
    const auto game = LocalGame::get_singleton();

    // Rendering:
    rs->instance_set_visible(instance_rid, false);
    //ERR_PRINT("Unlade Chunk.");
    
    game->remove_chunk(key);
}
void VG::ServerIntervention::UnLoadChunk::commit() {
    // nichts zu tun.
}




/*
void VG::ServerIntervention::UpdateData::add_to_commit() {
    if (r_col) {
        r_col->add_to_commit();
    }
    if (lod) {
        lod->add_to_commit();
    }
    if (c_col) {
        c_col->add_to_commit();
    }
    if (mesh) {
        mesh->add_to_commit();
    }
}
*/






void VG::ServerIntervention::LoadVoxelChunk::add_to_commit() {}
void VG::ServerIntervention::LoadVoxelChunk::commit() {}



void VG::ServerIntervention::OneShotTasks::load_and_generate_surrounding_chunks(const ChunkKoords center, const RADIUS radius, const VoxelGrid * const voxel_grid) {
    // TODO: evtl. Optimierung durch Multithreading bei größeren 'radius'. ACHTE AUF "add_to_commit"! (?)
    const auto cb = ChunkBuilder::get_singleton();
    const auto dim = voxel_grid->get_dimensions();
    ChunkKoords koords = dim.clamp(center - static_cast<short>(radius));
    ChunkKoords end = dim.clamp(center + static_cast<short>(radius));

    for (; koords.x < end.x; ++koords.x) {
        for (; koords.y < end.y; ++koords.y) {
            for (; koords.z < end.z; ++koords.z) {
                // load:
                ChunkMemoryUnit chunks = voxel_grid->summon_chunks_with_voxels(koords);
            }
            koords.z -= 2 * radius;
        }
        koords.y -= 2 * radius;
    }

    //TODO: hier könnte was falsche sein: äußersten Chunks 2-mal Mesh erzeugt?
    //TODO: das hier ist ein Hässlicher Workaround, damit die Chunks am Rand nicht 2-mal geMeshed werden...
    koords = dim.clamp(center - static_cast<short>(radius-1));
    end = dim.clamp(center + static_cast<short>(radius-1));
    for (; koords.x < end.x; ++koords.x) {
        for (; koords.y < end.y; ++koords.y) {
            for (; koords.z < end.z; ++koords.z) {
                ChunkMemoryUnit chunks = voxel_grid->summon_chunks_with_voxels(koords);
                // generate:
                for (ChunkContent *&content : chunks) {
                    // ERR_CONTINUE_MSG(content == nullptr, "Ein Chunk konnte nicht geladen/erzeugt werden.");
                    if (content) {
                        CreateMesh{
                            cb->generate_mesh_data(content->voxels),
                                content->get_mesh(),
                                content->get_instance(),
                                koords,
                                voxel_grid
                        }.add_to_commit();
                    }
                }
                CreateMesh::commit();
            }
            koords.z -= 2 * radius;
        }
        koords.y -= 2 * radius;
    }
}
void VG::ServerIntervention::ThreadSaveTasks::create_mesh(ThreadDataTransmitter<CreateMesh> &data_transmitter, ChunkKoords chunk_koords,  const VoxelGrid * const voxel_grid) {
    const auto cb = ChunkBuilder::get_singleton();
    const auto game = LocalGame::get_singleton();
    const ChunkContent *chunk_content = game->get_chunk(chunk_koords, voxel_grid->get_instance_id());

    if (chunk_content && chunk_content->voxels) {
        data_transmitter.queue_and_transmit_overflowing_task(CreateMesh{
            cb->generate_mesh_data(chunk_content->voxels),
            chunk_content->get_mesh(),
            chunk_content->get_instance(),
            chunk_koords,
            voxel_grid });
    }    
}
void VG::ServerIntervention::ThreadSaveTasks::delete_mesh(ThreadDataTransmitter<CreateLOD1> &data_transmitter, ChunkKoords chunk_koords,  const VoxelGrid * const voxel_grid) {
    const auto game = LocalGame::get_singleton();
    const ChunkContent *chunk_content = game->get_chunk(chunk_koords, voxel_grid->get_instance_id());

    if (chunk_content) {
        data_transmitter.queue_and_transmit_overflowing_task(CreateLOD1{
            chunk_content->get_mesh() });
    }
}
//TODO: das hier ist kein ThreadSaveTask, hier werden Chunkgs gepushed!
void VG::ServerIntervention::ThreadSaveTasks::load_chunks_with_voxles_from_disk(ThreadDataTransmitter<LoadChunk> &data_transmitter, ChunkKoords chunk_koords,  const VoxelGrid * const voxel_grid) {
    // const auto cb = ChunkBuilder::get_singleton();
    ChunkMemoryUnit chunks = voxel_grid->summon_chunks_with_voxels(chunk_koords);
    // TODO: hier passiert nichts auf dem MainThread... stimmt das?
    /*
    for (ChunkContent*& content : chunks) {
        if (content) {
            vec.emplace_back(
                LoadChunk{}
            );
        }
    }
    */
}
void VG::ServerIntervention::ThreadSaveTasks::load_chunk_and_create_LOD(ThreadDataTransmitter<LoadChunk> &data_transmitter, ChunkKoords chunk_koords,  const VoxelGrid * const voxel_grid) {
    // const auto cb = ChunkBuilder::get_singleton();
    ChunkMemoryUnit chunks = voxel_grid->summon_chunks(chunk_koords);
    for (ChunkContent *&content : chunks) {
        if (content) {
            //TODO: create LOD!
        }
    }
}
void VG::ServerIntervention::ThreadSaveTasks::remove_chunk_and_save_to_disk(ThreadDataTransmitter<UnLoadChunk> &data_transmitter, ChunkKoords chunk_koords,  const VoxelGrid * const voxel_grid) {
    const auto mm = MemoryManager::get_singleton();
    // remove:
    ChunkKey key{ chunk_koords, voxel_grid->get_instance_id() };
    const ChunkContent *chunk_content = LocalGame::get_singleton()->get_chunk(key);
    if (chunk_content) {
        data_transmitter.queue_and_transmit_overflowing_task(UnLoadChunk{ key, chunk_content->get_instance() });
    }
    // save:

    mm->queue_save_chunk_to_disk(ChunkKey{chunk_koords, voxel_grid->get_instance_id()});
    // TODO: hier kommt evtl. nochwas...
}
void VG::ServerIntervention::ThreadSaveTasks::generate_collision_PG_box(ThreadDataTransmitter<CreateCollision> &data_transmitter, PGKoords closest_PG_pos,  const VoxelGrid * const voxel_grid) {
    const auto cb = ChunkBuilder::get_singleton();
    const auto game = LocalGame::get_singleton();
    ChunkPhysics &collision_data = voxel_grid->get_physics();
    
    const ChunkKoords chunk_koords = ChunkKoords::from((closest_PG_pos / static_cast<real_t>(CHUNK_SIZE)).floor());
    // ignoriere ShapeHolder in BEKANNTEN leeren Chunks:
    if (collision_data.is_empty_chunk(chunk_koords)) {
        // ERR_PRINT(("chunk ist bekannt leer: " + chunk_koords.to_str()).c_str());
        return;
    }
    const ChunkContent* chunk_content = game->get_chunk(chunk_koords, voxel_grid->get_instance_id());
    // Chunk existiert gar nicht -> fühge bisher unbekannten leerer Chunk hinzu
    if (chunk_content == nullptr) {
        // ERR_PRINT(("neuer leerer Chunk: " + chunk_koords.to_str()).c_str());
        collision_data.add_empty_chunk(chunk_koords);
        return;
    }
    
    //TODO: Fall 0. 'fast_collision' zeigt ein leeres 64-tel
    // if (chunk_content.fast_collision...)
    ERR_FAIL_NULL_MSG(chunk_content->voxels, ("Chunk " + chunk_koords.to_str() + " besitzt keine Voxel. Konnte keine CollisionShapes berechnen.").c_str());
    const VoxelKoords voxel_koords = VoxelKoords::from(closest_PG_pos) - VoxelKoords::from(chunk_koords * CHUNK_SIZE);
    const CollisionData data{ voxel_koords, chunk_content->voxels };

    BoxShapeContainer shapes = cb->generate_collision_shapes(data);

    if (!shapes.empty()) {
        data_transmitter.queue_and_transmit_overflowing_task(CreateCollision{
            closest_PG_pos,
            voxel_grid,
            shapes });
    } else {
        //TODO: von der Logik her sollte das hier einen Funktionsaufruf höher passieren, aber performativ, ist es hier besser...
        // ERR_PRINT("add_empty_shape_holder.");
        collision_data.add_empty_shape_holder(EmptyShapeHolder{closest_PG_pos});
    }
}
void VG::ServerIntervention::ThreadSaveTasks::remove_collision_PG_box(ThreadDataTransmitter<RemoveCollision> &data_transmitter, PGKoords closest_PG_pos, const VoxelGrid *const voxel_grid) {

}
/*
void VG::ServerIntervention::ThreadSaveTasks::load_chunk_with_content_from_disk(ThreadDataTransmitter<LoadContentData> &data_transmitter, ChunkKoords chunk_koords, const VoxelGrid *const voxel_grid) {
    // TODO: hier müsste jetzt ein Chunk geladen werden

    
}
*/