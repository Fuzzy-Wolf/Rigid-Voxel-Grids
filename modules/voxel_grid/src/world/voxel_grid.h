#ifndef VOXEL_GRID_H
#define VOXEL_GRID_H

#include "core/error/error_macros.h"
#include "core/math/math_defs.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "core/math/vector3i.h"
#include <cassert>
#include <cstdint>
#include <semaphore>
#include <string>
#include <vector>

#include "core/object/object_id.h"
#include "modules/voxel_grid/src/meta/MemoryManager.h"
#include "modules/voxel_grid/src/meta/VoxelGridGenerator.h"
#include "scene/3d/physics/physics_body_3d.h"
#include "../meta/VoxelGridGenerator.h"

#include "../global.h"
#include "../utils.h"


namespace VG {
// TODO: besserer Name hierfür!
// TODO beobachten...: für Mulitplayer sollte nicht jeder Klient für alles seine Physik lokal berechnen.
// Die Positionen, um denen herum Shapes gebildet werden müssen.

class ShapeHolder {
    //TODO: std::vector<Vector3> local_pos; Mit Vector kann ich mehrer ShapeHolder verbinden, um Shapes zusammen zu fassen!
    RID _first_shape;
    uint8_t _shape_count = 0;
    // TODO: '_keep_this' muss nicht atomic sein, WENN alles auf dem selben Thread läuft.
//    mutable bool _keep_this = false;   // gibt an ob dieser ShapeHolder freigegeben werden kann, oder bleiben muss
    PGKoords _voxel_koords_in_grid;
public:
    ShapeHolder(PGKoords koords, RID first_shape, uint8_t shape_count)
        : _first_shape(first_shape), _shape_count(shape_count), _voxel_koords_in_grid(koords) {}
    inline bool has_shapes() const {
        return _first_shape.is_valid();
    }
    inline RID get_first_shape_rid() const {
        return _first_shape;
    }
    inline void set_first_shape_rid(RID shape_rid) {
        _first_shape = shape_rid;
    }
    inline PGKoords get_koords() const {
        return _voxel_koords_in_grid;
    }
    inline uint8_t get_shape_count() const {
        return _shape_count;
    }
};

struct CharacterShapeHolder {
    static const uint8_t SIZE = 8;

    CharacterShapeHolder() = delete;
    CharacterShapeHolder(PGKoords koords) : start_koords(koords) {}

    PGKoords get_koords(uint8_t index) {
        /* [0] -> (z: 0 y:0 x: 0)       [1] -> (z: 0 y:0 x:-a)       [2] -> (z: 0 y:-a x: 0)       [3] -> (z: 0 y:-a x:-a)
         * [4] -> (z:-a y:0 x: 0)       [5] -> (z:-a y:0 x:-a)       [6] -> (z:-a y:-a x: 0)       [7] -> (z:-a y:-a x:-a)
         */
        PGKoords koords = start_koords;
        switch (index) {
            case 7:
                koords.x -= PG_SIZE;
            case 6:
                koords.y -= PG_SIZE;
            case 4:
                koords.z -= PG_SIZE;
                break;
            case 3:
                koords.y -= PG_SIZE;
            case 1:
                koords.x -= PG_SIZE;
                break;
            case 5:
                koords.x -= PG_SIZE;
                koords.z -= PG_SIZE;
                break;
            case 2:
                koords.y -= PG_SIZE;
                break;
        }
        return koords;
    }
    inline bool is_missing(uint8_t index) {
        return !found[index];
        //return missing_arr.get(index);
    }
    inline void set_found(const PGKoords &koords) {
        uint8_t index = (start_koords.x == (koords.x + PG_SIZE)) + 2 * (start_koords.y == (koords.y + PG_SIZE)) + 4 * (start_koords.z == (koords.z + PG_SIZE));
        found[index] = true;
        //missing_arr.unset(index);
        count += 1;
    }
    inline bool contains(const PGKoords &koords) const {
        return (start_koords.x - PG_SIZE <= koords.x && koords.x <= start_koords.x) &&
               (start_koords.y - PG_SIZE <= koords.y && koords.y <= start_koords.y) &&
               (start_koords.z - PG_SIZE <= koords.z && koords.z <= start_koords.z);
    }
    inline uint8_t get_count() const {
        return count;
    }
private:
    const PGKoords start_koords;
    //Bit8Array missing_arr{ 0xFF };
    std::array<bool, SIZE> found{};
    uint8_t count = 0;
};

struct EmptyShapeHolder {
    PGKoords koords;
};

class GridDimension {
    //TODO: weiter optimieren: neben empty Chunks könnte man hier riesige Abschnitte (mehrere Chunks zusammen gefasst) als leer makieren.
    // z. B. Sonderfälle wie L-Form oder Ähnliches abdecken

//    mutable std::mutex mutex;
    //TODO: uint8_t reicht nur wenn ich den Origin und damit auch ALLE CHUNK-KOORDS verschiebe! aber fürs erste solls reichen... (ich könnte vorerst die Grid größe auf 255 begrenzen)
    ChunkKoords top_corner{1, 1, 1}; // [Chunks]
    ChunkKoords bottom_corner{0, 0, 0}; // [Chunks]

public:
    bool contains(VKoords koords) const {
//        const std::lock_guard<std::mutex> lock(mutex);
        return maxnorm_distance_to_border(koords) <= 0;
    }
    // Punkt ist innerhalb wenn "result" <= 0 , sonst außerhalb.
    short maxnorm_distance_to_border(VKoords koords) const {
        // const std::lock_guard<std::mutex> lock(mutex);
        koords = distance_to_border(koords);
        return koords.max();
    }
    // berechnet den Abstand zum Rand der BB in allen 3 Koordinaten. [Voxel]
    VKoords distance_to_border(VKoords koords) const {
        //TODO: VOXEL_SIZE fehlt hier...
        if (koords.x <= 0) {
            koords.x = static_cast<short>(-koords.x);
            koords.x += (CHUNK_SIZE * bottom_corner.x);
        } else {
            koords.x -= CHUNK_SIZE * top_corner.x;
        }
        if (koords.y <= 0) {
            koords.y = static_cast<short>(-koords.y);
            koords.y += (CHUNK_SIZE * bottom_corner.y);
        } else {
            koords.y -= CHUNK_SIZE * top_corner.y;
        }
        if (koords.z <= 0) {
            koords.z = static_cast<short>(-koords.z);
            koords.z += (CHUNK_SIZE * bottom_corner.z);
        } else {
            koords.z -= CHUNK_SIZE * top_corner.z;
        }
        return koords;
    }
    void try_grow(ChunkKoords koords) {
        ChunkKoords pos_dif = (koords + 1) - top_corner; // (koords + 1): "top_corner" von 'koords'
        top_corner += pos_dif & (ChunkKoords{0} < pos_dif);
        ChunkKoords neg_dif = koords - bottom_corner;
        bottom_corner += neg_dif & (neg_dif < ChunkKoords{ 0 });
        // ERR_PRINT(("top_corner: " + top_corner.to_str()).c_str());
        // ERR_PRINT(("bottom_corner: " + bottom_corner.to_str()).c_str());
    }

    ChunkKoords ceil(ChunkKoords koords) const {
        return min(koords, top_corner - 1);
    }
    ChunkKoords floor(ChunkKoords koords) const {
        return max(koords, bottom_corner);
    }
    ChunkKoords clamp(ChunkKoords koords) const {
        return max(min(koords, top_corner - 1), bottom_corner);
    }
    PGKoords clamp(PGKoords koords) const {
        return max(min(koords, PGKoords::from(top_corner * CHUNK_SIZE)), PGKoords::from(bottom_corner * CHUNK_SIZE));
    }
};

class ChunkPhysics {
    /********************* Physics Daten *********************/
    RingContainer<EmptyShapeHolder,128> empty_shape_holders{ EmptyShapeHolder{-32000} }; // TODO: das hier nochmal angucken. LargeGrids brauche eine große Zahl, aber kleine Grids nicht...
    FastContainer<ShapeHolder> shape_holders; // TODO: der Container sollte auch wieder Schrumpfen können
    std::vector<ShapeHolder> new_shape_holders;
    std::vector<ShapeHolder> deleted_shape_holders; //TODO: hiermit kann man vieleicht mehr tun, wie: ShapeHolder reaktivieren, löschen sobalt Rechenzeit übrig ist, etc...

    RingContainer<ChunkKoords> empty_chunks{ -32000 }; // müssen mit invaliden daten initialisiert werden
    //TODO: vieleicht ist ein atomic_bool besser als binary_semaphore...
    //    std::binary_semaphore semaphore{0}; // 0: gesperrt | 1: verfügbar

public:
    std::binary_semaphore physics_update{ 0 };
    
    ChunkPhysics() {
        shape_holders.reserve(SHAPEHOLDER_STORAGE_SIZE);
        new_shape_holders.reserve(8);
        deleted_shape_holders.reserve(32);
    }

    inline void add_empty_chunk(const ChunkKoords koords) {
        ERR_PRINT(("add_empty_chunk: " + koords.to_str()).c_str());
        empty_chunks.add(koords);
    }
    inline bool is_empty_chunk(const ChunkKoords koords) {
        return empty_chunks.has(koords);
    }
    inline FastContainer<ShapeHolder>::iterator remove_shape_holder(const FastContainer<ShapeHolder>::iterator& shape_holder) {
        deleted_shape_holders.emplace_back(*shape_holder);
        return shape_holders.erase(shape_holder);
    }
    inline std::vector<ShapeHolder>& get_deleted_shape_holders() {
        return deleted_shape_holders;
    }
    inline void add_empty_shape_holder(const EmptyShapeHolder &&shape_holder) {
        empty_shape_holders.add(shape_holder);
    }
    // gibt von den 8 (2x2x2) ShapHoldern um einen Charakter die zurück, die nicht existieren (binärhochzählend [0]: 0,0,0 | [1]: 0,0,1 | [7]: 1,1,1)
    inline const CharacterShapeHolder find_shape_holders_around_PGKoords(const PGKoords closest_PG_koords) {
        CharacterShapeHolder found_shape_holders{ closest_PG_koords };
        for (const ShapeHolder &shape : shape_holders) {
            if (found_shape_holders.contains(shape.get_koords())) {
                found_shape_holders.set_found(shape.get_koords());
                if (found_shape_holders.get_count() == CharacterShapeHolder::SIZE) {
                    break;
                }
            }
        }
        for (const EmptyShapeHolder &empty_shape : empty_shape_holders) {
            if (found_shape_holders.contains(empty_shape.koords)) {
                found_shape_holders.set_found(empty_shape.koords);
                if (found_shape_holders.get_count() == CharacterShapeHolder::SIZE) {
                    break;
                }
            }
        }
        // ERR_PRINT(("Es wurden " + std::to_string(count) + " viele SH um Spieler gefunden.").c_str());
        return found_shape_holders;
    }
    inline void add_shape_holder(const ShapeHolder &&shape_holder) {
        new_shape_holders.emplace_back(shape_holder);
    }
    // Warnung: sollte immer so wenig wie möglich aufgerufen werden. MUSS aufgerufen werden, BEVOR neue ShapeHolder erstellt wurden, sonst undefiniertes Verhalten!
    inline FastContainer<ShapeHolder> &get_shape_holders() {
        if (physics_update.try_acquire()) {
            for (ShapeHolder& new_sh : new_shape_holders) {
                //ERR_PRINT("füge neuen SH hinzu...");
                shape_holders.emplace(new_sh);
                //ERR_PRINT(("shape_holders hat jetzt länge: " + std::to_string(shape_holders.size())).c_str());
            }
            new_shape_holders.clear();
        } else {
            ERR_PRINT("physics_update-semaphore hat nicht funkioniert...");
        }
        return shape_holders;
    }
};

struct GridData {
    GridDimension extent; // [Chunks] maximale Ausdehnung des VoxelGrids (auch "BB" oder "BoundingBox") // TODO: einheitlicher Name
    Vector3 mass_center;
    uint64_t mass = 0;
    // ...
};

class VoxelGrid {
    static RADIUS distance_player_chunk(const Vector3i &Chunk_koords, const Vector3i &player_koords);
    // gibt die Chunk-Kooridnaten des Voxel-beinhaltenden Chunks aus.
    ChunkKoords get_chunk_koords_from_local_voxel_pos(const Vector3 &vox_pos_in_grid); //TODO: brauch ich das noch!?
    VoxelKoords get_voxel_koords(const Vector3 vox_pos_in_grid, const ChunkKoords chunk_koords); //TODO: brauch ich das noch!?

    //TODO: 'last_player_koords' in VoxelGridGenerator stecken, da nur generierte VG das brauchen.
    mutable PersistentThreadData thread_data;

    //TODO: ...
    VoxelGridGenerator* generator = nullptr;
    
protected:
    VoxelGrid(PhysicsBody3D* body) noexcept;
    VoxelGrid(PhysicsBody3D *body, ushort radius) noexcept;
    VoxelGrid() = delete;
    ~VoxelGrid() {
        delete generator; // new_delete PlanetGenerator
    }
    
    // static void _body_moved(RID mesh_instance_rid) {
    //     RenderingServer::get_singleton()->instance_set_transform(mesh_instance_rid, state->get_transform());
    // }

    GridData data;

    mutable ChunkPhysics physics_shapes;
    RID physics_body_rid;
    
    PhysicsBody3D *const body_data;
public:
    
    
    // Gibt die Koordinaten des Chunks aus, der den Punkt 'pos' beinhaltet.
    ChunkKoords containing_chunk_koords(const Vector3 &pos) const;
    // Gibt die Koordinaten des Chunks aus, der den Punkt 'pos' am nächsten ist. (Chunk-origin ist in der Ecke (0,0,0))
    ChunkKoords snap_to_chunk_koords(const Vector3 &pos) const;
    // gibt die Position des nächsten PhysicsGridPunktes (PG) in Grid-weiten VoxelKoords aus. Durch subtrahieren von PG_SIZE (in jeder Dimension) können die anderen 7 nächsten PGs ermittelt werden.
    PGKoords snap_to_PG_koords(const Vector3 &pos) const;
    // gibt die Distanz zur Bounding-Box aus.
    real_t distance_to_BB(const Vector3 &pos) const;
    // gibt zurück, ob die übergebene Position in wechselwirkungs-reichweite ist.
    bool is_close(const Vector3 &pos) const;

    const Transform3D to_global_transform(ChunkKoords local_koords) const;
    const Transform3D to_global_transform(PGKoords local_koords) const;


    template<TaskID ID>
    inline auto& get_thread_data() const {
        return std::get<ID>(thread_data.data);
    }
    
    inline Vector3 get_position() const {
        return body_data->get_position();
    }
    inline Transform3D get_transform() const {
        return body_data->get_transform();
    }
    inline const GridDimension& get_dimensions() const {
        return data.extent;
    }
    //TODO: eigene ID
    inline ObjectID get_instance_id() const {
        return body_data->get_instance_id();
    }
    inline const RID& get_body_rid() const {
        return physics_body_rid;
    }


    inline void add_chunk(const ChunkKoords koords) {
        // TODO: ...
        data.extent.try_grow(koords);
    }

    inline ChunkPhysics& get_physics() const {
        return physics_shapes;
    }

    ChunkMemoryUnit summon_chunks_with_voxels(ChunkKoords koords) const;
    ChunkMemoryUnit summon_chunks(ChunkKoords koords) const;
    /*
    void remove_block(Vector3 global_col_pos, Vector3 normal);
    void add_block(Vector3 global_col_pos, Vector3 normal);
    */


};
}; //namespace VVG

using VG::VoxelGrid;

#endif // VOXEL_GRID_H