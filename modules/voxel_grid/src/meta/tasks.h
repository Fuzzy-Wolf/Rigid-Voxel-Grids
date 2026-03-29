#ifndef THREAD_TASKS_H
#define THREAD_TASKS_H

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include "core/error/error_macros.h"
#include "core/math/vector3.h"
#include "core/math/vector3i.h"
#include "core/typedefs.h"

#include "../world/chunk.h"
#include "../world/local_game_instance.h"
#include "../world/Character.h"
#include "../global.h"
#include "../world/voxel_grid.h"
#include "../utils.h"
#include "../Schemas.h"
#include "ServerIntervention.h"
#include "modules/voxel_grid/src/meta/shared_data.h"

namespace VG {


namespace _TASKS_ {

// template <typename T>
// concept ThreadDataTransmitter_t = specialization_of<T, ThreadDataTransmitter>;



template <TaskID ID, typename... Data>
requires (DataScheme<Data> && ...)
class TaskDataQueues {
    virtual void _task() = 0;
    
protected:

public:
    using Types = std::tuple<Data...>;
    std::tuple<ThreadDataTransmitter<Data>...> pipelines;

    // (Dekorator-pattern)
    void operator()() {
        _task();
        if constexpr (std::tuple_size_v<Types> != 0) {
            std::apply([&](auto&&... pipeline) {
                (pipeline.transmit_queued_tasks(), ...);
            }, pipelines);
        }
    }
    void after_task() {
        std::apply(
            [&](auto &&...pipeline) {
                (pipeline.consume_all_transmitted_tasks(), ...);
        },pipelines);
    }
    constexpr TaskID get_id() { return ID; }
    TaskDataQueues() = default;
};


// RADIUS in Chunks
template <TaskID ID, RADIUS R, typename Data_in, typename Data_out, ServerIntervention::ThreadSaveTasks::TaskFunction<Data_in> F_in, ServerIntervention::ThreadSaveTasks::TaskFunction<Data_out> F_out>
class RadiusTask : public TaskDataQueues<ID, Data_in, Data_out> {
    enum PIPELINE {
        IN = 0,
        OUT = 1,
    };

public:
    using Task = TaskDataQueues<ID, Data_in, Data_out>;
    RadiusTask() {}

    virtual void _task() override {
        for (const ThreadData &voxel_grid : voxel_grid_infos) {
            const ChunkKoords& closest_chunk_koords = voxel_grid.current_player_koords;
            const ChunkKoords& closest_chunk_koords_before = voxel_grid.old_player_koords;

            //TODO: Check ob Ende des Grids erreicht / ob das Grid zu weit weg ist! KANN NICHT mit 'clamp' alleine gemacht werden!! statt dessen muss die entsprechende Richtung deaktiviert werden und für alle anderen geclampt werden.
    
            // berechne start- und end-Indices, wo die Chunks berechnet werden müssen
            const ChunkKoords start = closest_chunk_koords_before - ChunkKoords{R};
            const ChunkKoords end   = closest_chunk_koords_before + ChunkKoords{R};
            // wie viele Chunks sich der Spieler (die Spielerkamera) bewegt hat
            const ChunkKoords dif   = closest_chunk_koords - closest_chunk_koords_before;
    
            Indizes ranges = Indizes(start, end, dif);
    
            // diff muss ungleich 0 sein...
            int abs_x = abs(dif.x);
            int abs_y = abs(dif.y);
            int abs_z = abs(dif.z);
            //TODO: ... und kleiner-gleich R. Fall "größer als R" abdecken! und nur ungleich 0 testen
                    
            if (0 < abs_x && abs_x <= R) {
                work_on_direction(dif, Vector3i::AXIS_X, ranges, voxel_grid.vg_ptr);
            }
            if (0 < abs_y && abs_y <= R) {
                work_on_direction(dif, Vector3i::AXIS_Y, ranges, voxel_grid.vg_ptr);
            }
            if (0 < abs_z && abs_z <= R) {
                work_on_direction(dif, Vector3i::AXIS_Z, ranges, voxel_grid.vg_ptr);
            }
        }
    }
    // Erzeugt in allen VoxelGrids einen neuen Zustand ('State').
    // Wird von MainThread aufgerufen.
    void before_task() {
        const auto game = LocalGame::get_singleton();
        const Vector3 player_pos = game->get_player_pos();
        voxel_grid_infos.clear();
        for (auto &vg : game->get_large_grids()) {
            const ChunkKoords player_koords = vg->snap_to_chunk_koords(player_pos);
            ChunkKoords &last_player_koords = vg->get_thread_data<ID>();
            // hat sich der Spieler relativ zum Grid nicht bewegt, muss nichts passieren
            if (player_koords != last_player_koords) {
                //ERR_PRINT(("HUHU - player_koords: " + player_koords.to_str() + " - old_koords: " + vg->get_last_player_koords().to_str()).c_str());
                voxel_grid_infos.emplace_back(ThreadData{ vg, player_koords, last_player_koords });
            }
            last_player_koords = player_koords;
        }
        // if (voxel_grid_infos.size() != 0){
        //     ERR_PRINT(("'prepare_data' - meshing_data.size(): " + std::to_string(voxel_grid_infos.size())).c_str());
        // }
    }
protected:
    /**
     * @brief Behelfsklasse um Idizes zu Kapseln
     */
    class Indizes {
    public:
        const ChunkKoords start;
        const ChunkKoords end;
        ChunkKoords start_out;
        ChunkKoords   end_out;
        ChunkKoords start_in;
        ChunkKoords   end_in;

        Indizes() = delete;
        Indizes(const ChunkKoords &first, const ChunkKoords &last, const ChunkKoords &dif)
            : start(first), end(last), start_out(first), end_out(last), start_in(first + dif), end_in(last + dif)
        {}
    };
private:
    /**
     * @brief Führt die 'MeshUpdateFunc' auf einem Teil der relevanten Chunks aus.
     *        Mehrmaliges aufrufen auf allen 3 Axenrichtungen nötig, dabei die selben 'Idizes' verwenden!
     *
     * @param movement wie viele Chunks sich der Spieler bewegt hat
     * @param axis die Axen-Richtung, in der Chunks bearbeitet werden
     * @param ranges Kapselung aller Indices
     * @param voxel_grid Das VoxelGrid der Chunks
     */
    void work_on_direction(const ChunkKoords movement, const Vector3i::Axis axis, Indizes& indices, const VoxelGrid* const voxel_grid) {
        //TODO: lässt sich noch ein wenig verbessern. (einige Vars werden doppelt beschrieben...) [? na, ich denke es ist gut so wie es jetzt ist...]
        // Aliase nutzen um indices zu entpacken:
        ChunkKoords& start_out = indices.start_out;
        ChunkKoords&   end_out = indices.  end_out;
        ChunkKoords& start_in  = indices.start_in;
        ChunkKoords&   end_in  = indices.  end_in;
        const ChunkKoords& start = indices.start;
        const ChunkKoords&   end = indices.  end;
        
        if (0 < movement[axis]) {
            //     |.->.         |
            end_out[axis] = start[axis] + movement[axis];// - 1;
            //     |             |.->.
            start_in[axis] = end[axis];
        } else {
            //     |         .->.|
            start_out[axis] = end[axis] + movement[axis];
            // .->.|             |
            end_in[axis] = start[axis];// - 1;
        }

        // Chunks verlassen Radius:
        for (int16_t x = start_out.x; x < end_out.x; ++x) {
            for (int16_t y = start_out.y; y < end_out.y; ++y) {
                for (int16_t z = start_out.z; z < end_out.z; ++z) {
                    F_out(std::get<OUT>(this->pipelines), ChunkKoords{x, y, z}, voxel_grid);
                }
            }
        }

        // Chunks betreten Radius:
        for (int16_t x = start_in.x; x < end_in.x; ++x) {
            for (int16_t y = start_in.y; y < end_in.y; ++y) {
                for (int16_t z = start_in.z; z < end_in.z; ++z) {
                    F_in(std::get<IN>(this->pipelines), ChunkKoords{x, y, z}, voxel_grid);
                }
            }
        }

        // zurück setzen von 'start' und 'end'  UND  'Indizes' verschieben
        if (0 < movement[axis]) {
            start_out[axis] = start[axis] + movement[axis]; // 'Indizes'-Verschiebung
              end_out[axis] = end[axis];
             start_in[axis] = start_out[axis];
               end_in[axis] = end[axis];                    // 'Indizes'-Verschiebung
        } else {
            start_out[axis] = start[axis];
              end_out[axis] = end[axis] + movement[axis];   // 'Indizes'-Verschiebung
             start_in[axis] = start[axis];                  // 'Indizes'-Verschiebung
               end_in[axis] = end_out[axis];
        }
    }

    struct ThreadData {
        const VoxelGrid *vg_ptr;
        ChunkKoords old_player_koords;
        ChunkKoords current_player_koords;
        //TODO: dimension evtl. durch mutex in dimesnions abfrage ersetzen
        GridDimension dimension;
        ThreadData() = delete;
        ThreadData(const VoxelGrid *vg, ChunkKoords new_koords, ChunkKoords last_player_koords)
            : vg_ptr(vg)
            , old_player_koords(last_player_koords)
            , current_player_koords(new_koords)
            , dimension(vg->get_dimensions()) {}
    };
    
    std::vector<ThreadData> voxel_grid_infos;
};




//TODO: braucht eine vorgabe an Punkten, die verarbeitet werden sollen
template<TaskID ID>
class CharaktersPhysicsTask : public TaskDataQueues<ID, ServerIntervention::CreateCollision> {
    enum PIPELINE {
        CREATE = 0,
        REMOVE = 1,
    };

public:
    CharaktersPhysicsTask() : TaskDataQueues<ID, ServerIntervention::CreateCollision>() {}

    virtual void _task() override {
        for (const ThreadData& voxel_grid : voxel_grid_infos) {
            ChunkPhysics& collision_data = voxel_grid.vg_ptr->get_physics();
            std::vector<PGKoords> charc_koords;
            for (const Vector3& pos : character_positions) {
                PGKoords koords = snap_to_PG_koords(pos, voxel_grid);
                // beachte nur nahe Charaktere:
                if (is_close(koords, voxel_grid)) {
                    charc_koords.emplace_back(koords);
                    //characs.close_grids.try_remove_voxel_grid(voxel_grid.id);
                }
                //TODO: (2. Fall) für grid-to-grid collision sehr wichtig!
                // // - 2. Fall: der Character hat sich relativ zum VoxelGrid nicht weit genug bewegt:
                // if (character.close_grids.are_koords_still_the_same(voxel_grid.id, koords)) {
                //     continue;
                // }
            }

            /************* Freigeben von SapeHolder: *************/
            FastContainer<ShapeHolder>& shape_holders = collision_data.get_shape_holders();
            for (FastContainer<ShapeHolder>::iterator sh = shape_holders.begin(); sh != shape_holders.end();) {
                //TODO: Character_koords müssen nicht 2-mal "druch-loopt" werden. Aber es wird sehr verwoben und für's erste Reicht mir der Algorithmus so
                for (const PGKoords& koords : charc_koords) {
                    short dist = (sh->get_koords() - koords).abs().max(); // [PGKoords] 1-Norm
                    if (dist <= PG_deletion_distance) {
                        // behalte diesen ShapeHolder und gehe zum nächsten
                        ++sh;
goto _next_shape_holder_;
                    }
                }
                // kein nahen Charakter gefunden -> ShapeHolder freigeben:
                // ERR_PRINT("remove_shape_holder");
                sh = collision_data.remove_shape_holder(sh);
                //TODO: hier muss noch der MainThread mit einbezogen werden!
//... ich überlege, ob ich nicht den MainThread entscheiden lasse, wann die ShapeHolder entfernt werden müssen, und diese diese einfach direkt aus der deleteListe heraus nimmt... (= keine ThreadPipeline)
                // ThreadDataTransmitter<ServerIntervention::CreateCollision>& pipeline = std::get<CREATE>(this->pipelines);
                //         if (pipeline.container_full()) {
                //             pipeline.renew_container();
                //         }
                // ServerIntervention::ThreadSaveTasks::remove_collision_PG_box();
_next_shape_holder_:;
            }

            /************* Hinzufügen von SapeHolder: *************/
            for (PGKoords& koords : charc_koords) {
                CharacterShapeHolder csh = collision_data.find_shape_holders_around_PGKoords(koords);
                // nur wenn nicht alle 8 ShapeHolder gefunden wurden, gibt es was zu tun
                if (csh.get_count() < CharacterShapeHolder::SIZE) {
                    for (uint8_t i = 0; i < CharacterShapeHolder::SIZE; ++i) {
                        const PGKoords k = csh.get_koords(i);
                        short dist = distance_to_BB(k, voxel_grid);
                        if ( csh.is_missing(i) && dist <= 0 ) {
                            // ERR_PRINT(("generate_collision_PG_box: " + k.to_str()).c_str());
                            ServerIntervention::ThreadSaveTasks::generate_collision_PG_box(std::get<CREATE>(this->pipelines), k, voxel_grid.vg_ptr);
                        }
                    }
                }
            }
        }
    } // _task

    void before_task() {
        const auto game = LocalGame::get_singleton();
        voxel_grid_infos.clear();
        // TODO: large_voxel_grids zu ALLE VoxelGrids ändern
        for (auto &voxel_grid : game->get_large_grids()) {
            voxel_grid_infos.emplace_back(ThreadData{voxel_grid});
        }
        character_positions.clear();
        for (auto &character : game->get_characters()) {
            character_positions.emplace_back(character.get_position());
        }
    } // before_task

private:
    ushort PG_deletion_distance = PG_SIZE * 4; // [Voxel]
    std::vector<Vector3> character_positions;

    struct ThreadData {
        //TODO: dimension evtl. durch mutex in dimesnions abfrage ersetzen
        GridDimension dimension;
        Transform3D transform;
        const VoxelGrid *vg_ptr;
        ThreadData() = delete;
        ThreadData(const VoxelGrid *vg)
            : dimension(vg->get_dimensions())
            , transform(vg->get_transform())
            , vg_ptr(vg) {}
    };
    std::vector<ThreadData> voxel_grid_infos;

    //TODO: momentan unused...
    ChunkKoords containing_chunk_koords(const Vector3 &pos, const ThreadData &voxel_grid_info) {
        Vector3 local_pos = voxel_grid_info.transform.xform_inv(pos);
        local_pos /= (CHUNK_SIZE * VOXEL_SIZE);
        return ChunkKoords::from(local_pos.floor());
    }
    ChunkKoords snap_to_chunk_koords(const Vector3 &pos, const ThreadData &voxel_grid_info) {
        Vector3 local_pos = voxel_grid_info.transform.xform_inv(pos);
        local_pos /= (CHUNK_SIZE * VOXEL_SIZE);
        local_pos += Vector3(0.5, 0.5, 0.5);
        return ChunkKoords::from(local_pos.floor());
    }
    //...---------------------
    PGKoords snap_to_PG_koords(const Vector3 &pos, const ThreadData &voxel_grid_info) {
        Vector3 local_pos = voxel_grid_info.transform.xform_inv(pos);
        local_pos /= (PG_SIZE * VOXEL_SIZE);
        local_pos += Vector3(0.5, 0.5, 0.5);
        return PGKoords::from(local_pos.floor() * (PG_SIZE * VOXEL_SIZE));
    }
    short distance_to_BB(const VKoords &koords, const ThreadData &voxel_grid_info) {
        return voxel_grid_info.dimension.maxnorm_distance_to_border(koords);
    }
    bool is_close(const VKoords &koords, const ThreadData &voxel_grid_info) {
        return distance_to_BB(koords, voxel_grid_info) <= PG_SIZE;
    }
};

}; //namespace _TASKS_


using LoadingTask = _TASKS_::RadiusTask <TaskID::LOADING,
    RADIUS::DRAW_FULL,
    ServerIntervention::CreateMesh,
    ServerIntervention::CreateLOD1,
    ServerIntervention::ThreadSaveTasks::create_mesh,
    ServerIntervention::ThreadSaveTasks::delete_mesh>;

using MeshingTask = _TASKS_::RadiusTask<TaskID::MESHING,
    RADIUS::LOAD,
    ServerIntervention::LoadChunk,
    ServerIntervention::UnLoadChunk,
    ServerIntervention::ThreadSaveTasks::load_chunks_with_voxles_from_disk,
    ServerIntervention::ThreadSaveTasks::remove_chunk_and_save_to_disk>;

using CharaktersPhysicsTask = _TASKS_::CharaktersPhysicsTask<TaskID::PHYSICS>;


}; //namespace VG

#endif // THREAD_TASKS_H