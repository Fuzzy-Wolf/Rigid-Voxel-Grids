#ifndef CHUNK_H
#define CHUNK_H

#include <atomic>
#include <cmath>
#include <cstddef>
#include <utility>
#include <sys/types.h>

#include "core/templates/rid.h"
#include "servers/rendering_server.h"

#include "../global.h"
#include "../utils.h"
#include "VoxelData.h"


namespace VG {

class LocalGameInstance;

struct ChunkKey {
    ChunkKoords koords;
    uint64_t grid_id = static_cast<uint64_t>(-1); // TODO: eigene id vergeben und diese nur 32bit nutzen lassen. (erstmal total egal! bringt aber vieleicht irgendwann etwas...)
};
struct ChunkRIDs {
    const RID mesh_instance_rid;
    const RID mesh_rid;
};


template <size_t SIZE> class ChunkMapV2;

class ChunkContent {
    friend class ChunkBuilder;
    friend class ChunkMapV2<AKTIVE_CHUNKS_STORAGE_SIZE>;

    ChunkContent(RenderingServer *rs, LocalGameInstance* game);
private:
    ChunkContent(const ChunkContent&) = delete;
    ChunkContent(const ChunkContent &&c) : rids(c.rids), voxels(c.voxels) {}
    // ChunkContent(const ChunkContent &chunk) :
    //     rids(chunk.rids)
    // {
    //     if (chunk.voxels) {
    //         voxels = new VoxelData{*chunk.voxels}; // new_delete voxels
    //     }
    // }

public:
    ChunkContent();
    
    // Bit4x4x4Array lod;
    // Bit4x4x4Array fast_collision;
    
    const ChunkRIDs rids;
    VoxelData *voxels = nullptr;

    ~ChunkContent() {
        // wird erst bei Spiel-Ende in ALLEN instancen aufgerufen
        if (voxels) {
            delete voxels; // new_delete voxels
        }
    }

    inline RID get_mesh() const {
        return rids.mesh_rid;
    }
    inline RID get_instance() const {
        return rids.mesh_instance_rid;
    }
    inline const VoxelData* get_voxels() const {
        return voxels;
    }
    inline VoxelData* get_voxels() {
        return voxels;
    }
};

/**
 * @brief ChunkV2 stellt den Header eines Chunks dar. Die eigentlichen Daten stehen in 'content'. (Der Header wird nie auf der Festplatte gespeichert oder von ihr geladen. (das wäre sinnlos))
 *        Um Padding zu vermeiden ist der 'ChunkKey' nicht als solcher hinterlegt, sonder die einzelnen Felder 'koords' und 'grid_id' liegen "lose" in der Klasse.
 */
class ChunkV2 {
    friend class ChunkMapV2<AKTIVE_CHUNKS_STORAGE_SIZE>;
    friend struct ChunkHeader;
    // ALLE Member sind const, bis ein neuer Chunk an die Stelle eines alten geschrieben wird! (daher kann ich sie nicht wirklich const machen...)
    /*const*/ std::atomic_bool _is_free = true; // erst free und dann alle RIDs (u.s.w.) löschen! //  TODO: nicht hier in 'Chunk' -> use Bit-Array am anfang der 'ChunkMap' [Searchdepth=32] erste Null ist der Freie Platz (TOTAL Overengeneered, also erstmal nicht machen!!!)
    //TODO: Diese atmic bools sind etwas... naja lasch angewendet. Das sollte ich mir irgendwann nochmal ansehen.
    /*const*/ std::atomic_bool has_changed_since_load = false;

    /******** ChunkKey: ********/
    /*const*/ ChunkKoords koords;
    /*const*/ uint64_t grid_id = static_cast<uint64_t>(-1);
    /******** ^^^^^^^^^ ********/

    // TODO: vielleicht kann ich Indizes (2 byte) statt Pointer speichern so könnte ich die Größe von Chunk zusammen mit bit-Masking auf 16 Byte reduzieren...
    // ... wenn ich das mache sollte ich den Indexzähler in der ChunkMap in 4er-packen Teilen.
    ChunkContent* /*const*/ content = nullptr;

    inline void free() {
        _is_free.store(true,std::memory_order_acq_rel);
    }
    ChunkV2(ChunkContent* con) : content(con) {}
    ChunkV2(const ChunkV2 &c) = delete;

    void initialize_with(ChunkContent* const cont) {
        _is_free = true;
        has_changed_since_load = false;
        grid_id = static_cast<uint64_t>(-1);
        content = cont;
    }

public:
    enum PUSH_STATE : char {
        NOT_POSSIBLE = 0, // false
        WAS_FREE = 1, // true
        ALREADY_THERE = 2 // true
    };
    ChunkV2() {}; // never use this (explicitly)!
    ChunkV2* operator&() = delete;
    const ChunkV2* operator&() const = delete;
    
    inline const ChunkKoords get_koords() const {
        return koords;
    }
    inline uint64_t get_grid_id() const {
        return grid_id;
    }
    inline const ChunkKey get_key() const {
        return ChunkKey{koords, grid_id};
    }
    inline ChunkContent* get_content() const {
        return content;
    }

    inline bool is_free() const {
        return _is_free.load(std::memory_order_relaxed); // 'is_free' darf übersehen werden (nicht schlimm, gibt genug Platz)
    }
    inline PUSH_STATE try_store_successful(const ChunkKey& chunk_key) {
        //TODO: wenn alter Chunk der selbe ist, dann sind auch die RIDs noch da!! (nicht nur die voxel)
        if (!is_free()) {
            if (*this == chunk_key) {
                return ALREADY_THERE;
            }
            return NOT_POSSIBLE;
        }
        if (*this != chunk_key) {
            koords = chunk_key.koords;
            grid_id = chunk_key.grid_id;
            //TODO: Der Speicher sollte nicht frei gegeben werden, nur um ihn dann wieder anzufordern!
            //TODO: vieleicht ist es sinnvoll Stellen, in denen Voxel sind, ein wenig zu vermeiden (die ersten 3 Treffer oder so) um es den "zurückkehrenden Chunks" nicht zu vermiesen...
            delete content->voxels; // new_delete voxels
            content->voxels = nullptr;
        }
        _is_free.store(false, std::memory_order_release);
        return WAS_FREE;
    }
    inline void force_store_on_non_free(const ChunkKey& chunk_key) {
        koords = chunk_key.koords;
        grid_id = chunk_key.grid_id;
        //TODO: Der Speicher sollte nicht frei gegeben werden, nur um ihn dann wieder anzufordern!
        delete content->voxels; // new_delete voxels
        content->voxels = nullptr;
    }

    inline void override_with(ChunkV2 &chunk) {
        has_changed_since_load.store(chunk.has_changed_since_load.load(std::memory_order_relaxed),std::memory_order_relaxed); // Ähnliche Begründung wie oben: Der selbe Thread pushed und removed.
        koords = chunk.koords;
        grid_id = chunk.grid_id;
        std::swap(content, chunk.content);
        _is_free.store(false, (std::memory_order_release));
    }
    inline bool is_valid() {
        return _is_free;
    }


    inline bool operator==(const ChunkKey &key) {
        return koords == key.koords && grid_id == key.grid_id;
    }
    inline bool operator==(const ChunkV2 &chunk) {
        return koords == chunk.koords && grid_id == chunk.grid_id;
    }
};


struct ChunkHeader {
    const bool is_free = false;
    const bool has_changed_since_load = false;
    const ChunkKoords koords;
    const uint64_t grid_id = static_cast<uint64_t>(-1);
    ChunkHeader(const ChunkV2 &cv2)
        : is_free(cv2._is_free)
        , has_changed_since_load(cv2.has_changed_since_load)
        , koords(cv2.koords)
        , grid_id(cv2.grid_id) {}
};

}; //namespace VG

#endif // CHUNK_H