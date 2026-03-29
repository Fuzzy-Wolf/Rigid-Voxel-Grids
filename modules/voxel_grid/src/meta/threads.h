#ifndef WORKER_THREADS_H
#define WORKER_THREADS_H

#include "core/error/error_macros.h"
#include "core/math/vector3i.h"
#include "core/os/thread.h"
#include "core/string/ustring.h"
#include "core/typedefs.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#include "../Schemas.h"


/*
- MainThread überprüft auch wie lange er braucht, vermindert falls notwendig den längsten Thread
- Queue mit Paketen, die von MainThread gezogen und von WorkerThread gepusht werden.
- Wenn Thread zu langsam (gesamtlaufzeit) -> RADIUS reduzieren
- MainThread guckt wie viele Pakete sind bereit und bearbeitet entsprechend viele
- Paket größe ist 8 * RADIUS
- Woraus bestehen Pakete? std::array? -> momentan std::vector


NUR COLLISION-THREAD (!?!?) -> Aufteilung: PhysicsThreads(Liste an Punkten (z. B. Spieler)), DrawingThreads(Radius)
- Arbeitsteilung der Collision nicht nur nach Chunks sondern nach Anzahl-Shapes!
- Collision nicht ganzer Chunk sondern 8³-Blöcke
*/

namespace VG {

/**
 * @brief Hilft beim zählen der Threads und weist jedem Thread eine eindeutige ID zu.
 * 
 */
class ThreadCounter {
    static uint count;

public:
    uint static get_next_number() {
        return count++;
    }
    uint static get_count() {
        return count;
    }
};

/**
 * @brief Interface für meine WorkerThreads.
 * 
 */
class Thread {
    // Kopiert und "friert" die benötigten Daten für den Thread ein, damit dieser auf ihnen Arbeiten kann.
    virtual void prepare_required_data() = 0;
protected:
    bool _terminate_thread = false;
    std::atomic_bool _is_pending = true;
    std::thread _thread;

public:
    Thread() {}
    virtual ~Thread() {}
    /**
     * @brief friert notwendige Daten für den Thread ein und lässt diesen erneut laufen.
     */
    inline void run_with_fresh_data() {
        ERR_FAIL_COND_MSG(_is_pending.load(std::memory_order_acquire) == false, "Thread wurde reaktiviert, obwohl er noch arbeitet!");
        // Alle relevanten Daten einfrieren!
        prepare_required_data();
        // Thread aktive machen
        _is_pending.store(false, std::memory_order_release);
        _is_pending.notify_one();
    }
    
    inline void terminate_thread() {
        _terminate_thread = true;
        if (_thread.joinable()) {
            _thread.join();
        }
    }
    
    inline bool is_ready_and_waiting() {
        return _is_pending.load(std::memory_order_acquire);
    }

    virtual void do_main_thread_part_of_task() = 0;

    virtual long get_delta_time() = 0;
	
};



template <typename TASK>
    requires TaskScheme<TASK>
class WorkerThread : public Thread {
    // static constexpr long MAX_TIME_PER_TASK_IN_MICROSEC = 1000; // TODO: verwenden um bei zu langer berechnung workload zu reduzieren (?)
/*
    template <size_t... I>
    _FORCE_INLINE_ void on_pipelines(std::index_sequence<I...>) {
        // using Data = std::tuple_element_t<I, typename TASK::Types>;
        (
            std::get<I>(task.pipelines).queue.consume_all([&](std::vector<std::tuple_element_t<I, typename TASK::Types>>*& container) {
                for (std::tuple_element_t<I, typename TASK::Types> &data : *container) {
                    data.add_to_commit();
                }
                std::tuple_element_t<I, typename TASK::Types>::commit();
                delete container; // new_delete Container
            }),
            ...
        );
    }
*/
protected:
    TASK task;

public:
    //TODO: Funktionspointer sind hier wahrscheinlich besser, auch wenn schwieriger und gefährlicher...
    virtual void prepare_required_data() override { task.before_task(); };
    virtual void do_main_thread_part_of_task() override {
        //TODO: in 'run_again_*' 'rein packen. ODER Schlüssel zurückgeben, um sicherzustellen, das diese Funktion vor "run_again" ausgeführt wird.
        task.after_task();
    }

    virtual long get_delta_time() override {
        return deltatime;
    }
    
    inline void start() {
        if (_thread.joinable()) {
            return; // Thread läuft bereits
        }
        
        _thread = std::thread([&]() {
            while (!_terminate_thread) {
                // warte auf (re)aktivierung
                _is_pending.store(true, std::memory_order_release);
                _is_pending.wait(true,std::memory_order_acquire);
                if (_terminate_thread) {
                    break;
                }

                // Begin Zeitpunkt...
                const auto begin_time = std::chrono::steady_clock::now();
                
                // Arbeite den Task ab:
                this->task();
                
                // ...berechne Laufzeit:
                const auto end_time = std::chrono::steady_clock::now();
                deltatime = std::chrono::duration_cast<std::chrono::microseconds>(end_time - begin_time).count();
            }
            // thread terminiert
            ERR_PRINT_ED(("Thread mit ID " + std::to_string(task.get_id()) + " wurde terminiert.").c_str());
        });
    }

protected:
   long deltatime = 0L;

public:

    virtual ~WorkerThread() { terminate_thread(); }
    WorkerThread() { ERR_PRINT(("Starte Thread mit TaskID: " + std::to_string(task.get_id())).c_str()); start(); }
};






/*


template <typename Data_in, typename Data_out, MeshUpdateFunc<Data_in> F_in, MeshUpdateFunc<Data_out> F_out>
class PhysicsThreadTask {
    static constexpr long SHAPES_PER_PACKAGE = 64;

    ThreadData<Data_in, SHAPES_PER_PACKAGE> pipeline_inside; // Shapes, die zum Ph.-Body hinzugefügt werden sollen
    ThreadData<Data_out, SHAPES_PER_PACKAGE> pipeline_outside; // Chunks von denen die Shapes verschwinden sollen

    void task_on_radius(const size_t thread_id, const VoxelGrid* voxel_grid) {
        const Vector3i local_player_koords = voxel_grid->get_current_player_koords(thread_id);
        const Vector3i last_local_player_koords = voxel_grid->get_last_player_koords(thread_id);

        //TODO: Check ob Ende des Grids erreicht / ob das Grid zu weit weg ist!

        const Vector3i dif   = local_player_koords - last_local_player_koords;

        //TODO: wie funktionier das jetzt hier...? ...ich will häpchenweise Shapedaten an den MainThread übermitteln...
        const auto cb = ChunkBuilder::get_singleton();

        std::vector<ShapeData> shape_data;
        Chunk* chunk =
        cb->generate_collision(shape_data, short x, short y, short z, )
        apply_function(dif, voxel_grid);

        // letzte Spieler-Position aktualisieren
        voxel_grid->save_last_player_koords(thread_id, local_player_koords);
    }



void generate_collision(std::vector<CreateColData> &vec, short x, short y, short z, const VoxelGrid *voxel_grid) {
    const auto game = LocalGameInstance::get_singleton();
    const auto cb = ChunkBuilder::get_singleton();
    Chunk *chunk = game->get_chunk(x, y, z, voxel_grid->get_instance_id());
    if (chunk) {
        // ERR_FAIL_COND_EDMSG(chunk->is_in_use(),"Chunk wird bereits von einem Thread genutzt!");
        // chunk->use();
        const Vector3 pos = Vector3(x, y, z) * CHUNK_SIZE  * VOXEL_SIZE;
        const auto shape_data = cb->generate_collision(chunk->voxels, pos);
        if (!shape_data.empty()) {
            vec.emplace_back(CreateColData{
                    voxel_grid->physics_body_rid,
                    shape_data,
                    chunk });
        }
    }
}
void remove_collision(std::vector<RemoveColData> &vec, short x, short y, short z, const VoxelGrid *voxel_grid) {
    const auto game = LocalGameInstance::get_singleton();
    Chunk *chunk = game->get_chunk(x, y, z, voxel_grid->get_instance_id());
    //TODO: collision sollte nicht gelöscht, sondern deaktiviert werden und erst bei größeren Abstand gelöscht werden
    if (chunk && chunk->shape_count != 0) {
        vec.emplace_back(RemoveColData{
                voxel_grid->physics_body_rid,
                chunk->get_first_shape_rid(),
                chunk->shape_count });
    }
}

protected:

};







template <typename Data_in, typename Data_out, PhysicsUpdateFunc<Data_in> F_in, PhysicsUpdateFunc<Data_out> F_out>
class ChunkPhysicsThread : public PhysicsThreadTask<Data_in, Data_out, F_in, F_out>, public Thread {


    virtual void _freeze_relevant_data() override {
        const auto game = LocalGameInstance::get_singleton();
        const Vector3 player_pos = game->player->get_position();
        //TODO: alle anderen Positionen, an denen CollShapes gebraucht werden
        for (auto &vg : *game->voxel_grids) {
            vg->save_current_player_koords(
                _id,
                vg->pos_to_koords_center(player_pos)
            );
        }
    }
};

*/



}; //namespace VVG

#endif // WORKER_THREADS_H
