#include "voxel_universe.h"
#include "core/error/error_macros.h"
#include "core/math/vector3.h"
#include "static_voxel_grid.h"
#include "local_game_instance.h"
#include "../meta/ServerIntervention.h"
#include "../global.h"
#include <sys/types.h>
#include <cmath>

void VG::Universe::_bind_methods() {
    ClassDB::bind_method(D_METHOD("add_planet", "position", "radius"), &Universe::add_planet);
}

void VG::Universe::add_planet(const Vector3 &pos, ushort radius) {
    ERR_FAIL_COND_MSG(radius < 0, "Radius des Planeten muss 0 < R < MAX_GRID_SIZE sein.");
    const auto game = LocalGame::get_singleton();
    // planet erstellen
    StaticVoxelGrid *planet = memnew(StaticVoxelGrid(radius));
    planet->set_position(pos);
    // TODO: hier steht jetzt nur ein Workaround, damit die Dimesion des Planeten nicht 0 ist:
    planet->add_chunk(ChunkKoords{static_cast<int16_t>( radius)});
    planet->add_chunk(ChunkKoords{static_cast<int16_t>(-radius)});
    // planet->set_rotation(Vector3(1,1,0));
    add_child(planet);
    // planet hinzufügen
    //TODO: wait for Threads!
    game->add_large_grid(planet);

    ChunkKoords player_koords = planet->snap_to_chunk_koords(game->get_player_pos());
    ServerIntervention::OneShotTasks::load_and_generate_surrounding_chunks(
        player_koords, RADIUS::LOAD, planet
    );//TODO: Chunks auf dem Rand werden wahrscheinlich doppelt erzeugt. Weil Loading 1 weiter geht als Drawing
}
