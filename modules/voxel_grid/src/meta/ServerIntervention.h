#ifndef THREAD_DATA_H
#define THREAD_DATA_H


#include "core/math/transform_3d.h"
#include "core/templates/rid.h"
#include "core/variant/array.h"

#include "../world/local_game_instance.h"
#include "../world/chunk.h"

#include "../global.h"
#include "modules/voxel_grid/src/Schemas.h"
#include "modules/voxel_grid/src/meta/shared_data.h"

namespace VG {

class VoxelGrid;

/**
 * @brief "ServerIntervention" beinhaltet alle Funktionen und damit verbundenen Objekte, die mit den "PhysicsServer3D" und "RenderingServer" kommunizieren.
 *        Es ist wichtig, dass diese hervorstehen, da sie UNBEDINGT auf dem MainThread ausgeführt werden müssen.
 * 
 */
namespace ServerIntervention {
    
/* --- Chunk verändert sein Aussehen: --- */
struct CreateMesh {
    const Array mesh_data;
    const RID mesh;
    const RID instance;
    const ChunkKoords chunk_kooooooords;
    const VoxelGrid* const voxel_grid;

    void add_to_commit();

    static void commit();
};

struct CreateLOD1 {
    const RID mesh;

    void add_to_commit();

    static void commit();
};


/* --- Chunk verändert seine Physik: --- */

struct CreateCollision {
    const PGKoords pg_koords;
    const VoxelGrid * const voxel_grid;
    const BoxShapeContainer shapes;

    void add_to_commit();

    static void commit();

    RID fetch_new_box_and_add_to_body(const size_t index, const Transform3D &transform) const;
};
struct RemoveCollision {
    const RID body;
    const RID first_shape;
    const ushort shape_count = 0;

    void add_to_commit();

    static void commit();
};


/* --- Chunk erscheint oder verschwindet: --- */
struct LoadChunk { // Chunk erscheint im Spiel
    //TODO: diese Klasse ist so nicht sinnvoll. Wenn ein Chunk geladen wird, sollte er auch für irgendwas gebraucht werden (LOD, Voxels, ...).
    void add_to_commit();

    static void commit();
};
struct LoadVoxelChunk { // Chunk erscheint im Spiel

    void add_to_commit();

    static void commit();
};


struct UnLoadChunk { // Chunk verschwindet aus Spiel
    const ChunkKey key;
    const RID instance_rid;

    void add_to_commit();

    static void commit();
};



/* --- Chunk wird nach Änderung geupdated: --- */
/*
struct UpdateData {
    FullMeshData *mesh = nullptr;
    LOD1MeshData *lod = nullptr;
    CreateColData *c_col = nullptr;
    RemoveColData *r_col = nullptr;

    void add_to_commit();

    static void commit() {}

    ~UpdateData() {
        delete mesh;
        delete lod;
        delete c_col;
        delete r_col;
    }
};
*/



struct OneShotTasks {
    static void load_and_generate_surrounding_chunks(const ChunkKoords chunk_koords, const RADIUS radius, const VoxelGrid * const voxel_grid);
};


struct ThreadSaveTasks {
    
    template <typename UpdateData>
    using TaskFunction = void (ThreadDataTransmitter<UpdateData>&, ChunkKoords, const VoxelGrid* const);

    // template<typename UpdateData>
    // using MeshUpdateFunc = UpdateData* (short, short, short, const VoxelGrid*);
    
    // static void change_mesh_to_LOD1() {
    // }
    // static void load_chunk_with_content(std::vector<LoadChunk> &vec, ChunkKoords chunk_koords, VoxelGrid *voxel_grid)
    // {
    //     voxel_grid->load_chunk_with_content_from_disk(vec, chunk_koords);
    // }
    static void create_mesh(ThreadDataTransmitter<CreateMesh>&, ChunkKoords chunk_koords, const VoxelGrid * const);
    static void delete_mesh(ThreadDataTransmitter<CreateLOD1>&, ChunkKoords chunk_koords, const VoxelGrid * const);

    static void load_chunks_with_voxles_from_disk(ThreadDataTransmitter<LoadChunk>&, ChunkKoords chunk_koords, const VoxelGrid *const);
    // wenn chunk bereits geladen werden nur die Voxel geladen. Sind Voxel ebenfalls geladen, passiert nichts.
    //void load_chunk_with_content_from_disk(ThreadDataTransmitter<LoadContentData>&, ChunkKoords chunk_koords, const VoxelGrid *const);
    static void load_chunk_and_create_LOD(ThreadDataTransmitter<LoadChunk>&, ChunkKoords chunk_koords, const VoxelGrid * const);
    static void remove_chunk_and_save_to_disk(ThreadDataTransmitter<UnLoadChunk>&, ChunkKoords chunk_koords, const VoxelGrid * const);

    static void generate_collision_PG_box(ThreadDataTransmitter<CreateCollision>&, PGKoords closest_PG_pos, const VoxelGrid * const);
    static void remove_collision_PG_box(ThreadDataTransmitter<RemoveCollision>&, PGKoords closest_PG_pos, const VoxelGrid *const);
};




}; // ServerIntervention

}; //namespace VVG

#endif // THREAD_DATA_H