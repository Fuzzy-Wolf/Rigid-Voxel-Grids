#include <cassert>
#include <cmath>
#include <vector>

#include "core/error/error_macros.h"
#include "scene/resources/material.h"

#include "../meta/thread_pool.h"

#include "voxel_universe.h"
#include "local_game_instance.h"

// leider geht das hier wegen godot nicht... kack g.dot
// std::unique_ptr<VG::LocalGameInstance> const VG::LocalGameInstance::singleton = std::make_unique<LocalGameInstance>();

VG::LocalGameInstance* const VG::LocalGame::singleton = new LocalGameInstance();


VG::LocalGameInstance::LocalGameInstance() {
}

void VG::LocalGameInstance::allocate_chunk_storage() {
    const auto ps = PhysicsServer3D::get_singleton();

    _all_large_grid_chunks = new ChunkStorage();

    unused_box_shapes.reserve(512);
    for (uint _ = 0; _ < 512; ++_) {
        unused_box_shapes.emplace_back(ps->box_shape_create());
    }
}

void VG::LocalGameInstance::set_world_3d(World3D* world) {
    ERR_FAIL_COND_MSG(_world3D != nullptr, "world3d kann nicht gesetzt werden, da bereits eine Welt3D gesetzt wurde. Das Universum ist vielleicht schon gestartet worden.");
    _world3D = world;
    ERR_FAIL_NULL_MSG(_world3D, "Nullptr ist keine World3d!..");
}


void VG::LocalGame::_bind_methods() {
//    ClassDB::bind_method(D_METHOD("set_player", "player"), &LocalGameInstance::set_player);
    ClassDB::bind_static_method("LocalGame", D_METHOD("set_player", "player"), &LocalGame::set_player);
//     ClassDB::bind_static_method("LocalGameInstance", D_METHOD("get_voxel_material"), &LocalGameInstance::get_voxel_material);
//     ClassDB::bind_static_method("LocalGameInstance", D_METHOD("set_voxel_material","p_material"), &LocalGameInstance::set_voxel_material);
// //    ClassDB::bind_method(D_METHOD("set_voxel_material","p_material"), &LocalGameInstance::set_voxel_material);
//     ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "BaseMaterial3D,ShaderMaterial", PROPERTY_USAGE_DEFAULT), "set_voxel_material", "get_voxel_material");

    ClassDB::bind_static_method("LocalGame", D_METHOD("start_game", "universe", "material"), &LocalGame::start_game);
//    ClassDB::bind_method(D_METHOD("start_game", "universe"), &LocalGameInstance::start_game);

    ClassDB::bind_static_method("LocalGame", D_METHOD("try_updating_all_voxel_grid_chunks"), &LocalGame::try_updating_all_voxel_grid_chunks);
//    ClassDB::bind_method(D_METHOD("try_updating_all_voxel_grid_chunks"), &LocalGameInstance::try_updating_all_voxel_grid_chunks);
    ClassDB::bind_static_method("LocalGame", D_METHOD("get_chunk_storage_fill_rate"), &LocalGame::get_chunk_storage_fill_rate);
//    ClassDB::bind_method(D_METHOD("get_chunk_storage_fill_rate"), &LocalGameInstance::get_chunk_storage_fill_rate);
}


void VG::LocalGame::start_game(Universe* const universe, const Ref<Material> &p_material) {
    singleton->set_world_3d(universe->get_world_3d().ptr());
    // Speicher anlegen:
    singleton->allocate_chunk_storage();
    singleton->_material = p_material;
    // TODO: generien von Planeten, Sternen, usw...

    ThreadPool::get_singleton()->start();
}


void VG::LocalGame::try_updating_all_voxel_grid_chunks() {
	//std::vector<Thread*> deffered_threads;
	const auto tp = ThreadPool::get_singleton();
    /*TODO-Plan
        Ich will Chunkveränderungen konsistent haben.
        Veränderungen passieren auf Main-Thread. WorkerThreads könnten daher veraltete Daten haben.
        Lösung:
        - Anpassung an Veränderung 2-mal durchführen.
        - erste-mal sofort
        - zweite-mal nachdem nach der ersten Anpassung ALLE WorkerThreads einmaldurchgelaufen sind (wie ein "flush" -> keine alten Daten mehr)
        Umsetzung:
        - "Protokoll" (Liste) aller Veränderungen/Anpassungen mit abhak-liste aller Threads
    */
	for (auto &thread : tp->threads) {
		bool deffer_this_thread = false;
        thread->do_main_thread_part_of_task();
		if (thread->is_ready_and_waiting()) {

			/* Code der Zwischen Threaddurchläufen passiert */

			/* -------------------------------------------------- */
			if (!deffer_this_thread) {
                thread->run_with_fresh_data();
            }
        }
	}
	// Voxel entfernen und hinzufügen:
    /* TODO: Code here */
    /*
    for (auto &data : tp->main_thread_storage.changing_chunks) {
		data.add_to_commit();
    }
	
    tp->main_thread_storage.changing_chunks.clear();
	for (auto &deffered_thread : deffered_threads) {
		msg += "Hint: Thread wurde verzögert fortgesetzt.\n";
		msg += deffered_thread->run_again_with_fresh_data();
    }
    */
}

