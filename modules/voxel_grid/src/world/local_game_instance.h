#ifndef AKTIVE_CHUNK_H
#define AKTIVE_CHUNK_H

#include <cassert>
#include <climits>
#include <cstdint>
#include <thread>
#include <vector>

#include "core/error/error_macros.h"
#include "core/math/vector3i.h"
#include "core/object/object.h"
#include "core/variant/variant.h"
#include "scene/3d/node_3d.h"
#include "ChunkMap.h"
#include "voxel_universe.h"
#include "../global.h"
#include "chunk.h"
#include "Character.h"


namespace VG {
class VoxelGrid;

class LocalGameInstance {
    friend class LocalGame;
    using ChunkStorage = ChunkMapV2<AKTIVE_CHUNKS_STORAGE_SIZE>;

    Node3D *_player = nullptr;
    World3D *_world3D = nullptr;
    Ref<Material> _material;

    FastContainer<Character> characters;

    void allocate_chunk_storage();
    void set_world_3d(World3D *world);

    void load_surrounding_chunks(VoxelGrid* voxel_grid, const Vector3& pos);

    std::vector<VoxelGrid *> _large_grids;// sollten eigendlich immer nur um die 1,2 vieleicht mal 4 oder so sein.. also sehr wenige.
    std::vector<VoxelGrid *> _small_grids; // könnten mehr werden.. so bis zu 100 oder so.
    //TODO: wo werden die _small_grid_chunks gespeichert?

    ChunkStorage *_all_large_grid_chunks = nullptr;

public:
    
    std::vector<RID> unused_box_shapes; // 'allocate_chunk_storage' reserves 128

    // Nur MainThread!!!
    inline const Vector3 get_player_pos() {
        return _player->get_position();
    }

    inline const std::vector<VoxelGrid *>& get_large_grids() {
        return _large_grids;
    }
    inline void add_large_grid(VoxelGrid * grid) {
        _large_grids.emplace_back(grid);
    }

    inline const std::vector<VoxelGrid *>& get_small_grids() {
        return _small_grids;
    }
    inline void add_small_grid(VoxelGrid * grid) { // TODO: small und large sollten sich auf vom Typ her unterscheiden
        _small_grids.emplace_back(grid);
    }

    inline RID get_physics_space() {
        ERR_FAIL_NULL_V_MSG(_world3D, RID(), "PhysicySpace war null. Wahrscheinlich wurde ein VoxelGrid erzeugt, bevor die GameInstance gestartet wurde ('start_game').");
        return _world3D->get_space();
    }
    inline RID get_draw_scenario() {
        return _world3D->get_scenario();
    }
    inline RID get_material() {
        return _material->get_rid();
    }
    inline const FastContainer<Character>& get_characters() {
        return characters;
    }
    inline void add_character(Node3D * character) {
        characters.emplace(character);
    }

    // inline ChunkV2 *get_chunk_ptr(const ChunkKoords koords, uint64_t id) {
    //     // TODO: wenn durch pushen ein Chunk getauscht wird ist dieser Pointer falsch! (aber sehr sehr unwahrscheinlich, dass es je passiert...)
    //     return chunks_with_voxeldata->get(ChunkKey{ koords, id });
    // }
    inline const ChunkContent* get_chunk(const ChunkKoords koords, const uint64_t id) {
        return _all_large_grid_chunks->get(ChunkKey{ koords, id });
    }
    inline const ChunkContent* get_chunk(const ChunkKey key) {
        return _all_large_grid_chunks->get(key);
    }
    inline bool has_chunk_changed(const ChunkKoords koords, const uint64_t id) {
        return _all_large_grid_chunks->get_has_changed(ChunkKey{ koords, id });
    }

    inline void remove_chunk(const ChunkKey key) {
        //TODO: diese Funktion ist problematisch, da Chunk::free den Chunk freigibt, ohne das Mesh zu deaktivieren
        _all_large_grid_chunks->remove(key);
    }

    inline ChunkContent *push_chunk(const ChunkKoords koords, const uint64_t id) {
        //TODO: mach dir nochmal gedanken zu thread-locking!
        static std::atomic_flag lock = ATOMIC_FLAG_INIT;
        while (lock.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        ChunkContent* content = _all_large_grid_chunks->push(ChunkKey{ koords, id });

        lock.clear(std::memory_order_release);
        return content;
    }
    inline ChunkContent *push_chunk(const ChunkKey& key) {
        //TODO: mach dir nochmal gedanken zu thread-locking!
        return _all_large_grid_chunks->push(key);;
    }

    LocalGameInstance();
    ~LocalGameInstance() {
        if (_all_large_grid_chunks) {
            delete _all_large_grid_chunks;
        }
    }
};

class LocalGame : public Object {
    GDCLASS(LocalGame, Object)

    static inline void set_player(Node3D *player_node) {
        singleton->_player = player_node;
        singleton->add_character(player_node);
    }
    static inline Node3D *get_player() {
        return singleton->_player;
    }

    static double get_chunk_storage_fill_rate() {
        return singleton->_all_large_grid_chunks->get_occupancy_rate();
    }
    // TODO: start_game sollte eine GameInsance oder so zurückgeben, damit nicht dinge auf einem nicht gestarteten Game gemacht werden können... (?)
    static void start_game(Universe* const universe, const Ref<Material> &p_material);
    static void try_updating_all_voxel_grid_chunks();

    static LocalGameInstance *const singleton;
    
protected:
    static void _bind_methods();

public:
    static inline LocalGameInstance * get_singleton() {
        //MY_MARKER_001
//        ERR_FAIL_NULL_V_MSG(singleton, singleton, "WTF!!!!!!! NYOOOOOOOOOOOOOOOOOOOOOOOH! _LocalGameInstance betrüüüüügt!!!!");
        return singleton;}
};

}; //namespace VG

using VG::LocalGame;

/*
const size_t a = sizeof(AktiveChunks::MyHashMap);
const size_t b = alignof(AktiveChunks::MyHashMap);
*/

#endif // AKTIVE_CHUNK_H