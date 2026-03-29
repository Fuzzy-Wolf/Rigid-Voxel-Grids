#include <cmath>

#include "servers/rendering_server.h"

#include "chunk.h"
#include "local_game_instance.h"


VG::ChunkContent::ChunkContent(RenderingServer *rs, LocalGameInstance* game)
    : rids(ChunkRIDs{rs->instance_create(), rs->mesh_create()})
{
    rs->instance_set_base(rids.mesh_instance_rid, rids.mesh_rid);
    rs->instance_set_scenario(rids.mesh_instance_rid, game->get_draw_scenario());
}
VG::ChunkContent::ChunkContent() : VG::ChunkContent(RenderingServer::get_singleton(), LocalGame::get_singleton()) {}
