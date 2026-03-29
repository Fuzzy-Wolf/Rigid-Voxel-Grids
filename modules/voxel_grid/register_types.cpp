#include "register_types.h"

#include "core/object/class_db.h"
#include "src/world/voxel_universe.h"
#include "src/world/static_voxel_grid.h"
#include "src/world/local_game_instance.h"

static_assert(202002L <= __cplusplus, "C++20 Standard benötigt!");

void initialize_voxel_grid_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    ClassDB::register_class<StaticVoxelGrid>();
    ClassDB::register_class<LocalGame>();
    ClassDB::register_class<Universe>();
}

void uninitialize_voxel_grid_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
   // Nothing to do here in this example.
}
