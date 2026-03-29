#ifndef SHARED_THREAD_DATA_H
#define SHARED_THREAD_DATA_H

#include "core/error/error_macros.h"
#include "modules/voxel_grid/src/global.h"
#include <cassert>
#include <memory>
#include <thread>
#include <utility>
#include <vector>
#include <boost/lockfree/spsc_queue.hpp>
using boost::lockfree::spsc_queue;

#include "core/typedefs.h"
#include "modules/voxel_grid/src/Schemas.h"

namespace VG {



// TODO: verbesserbar, wenn Container aus Vorrat kommen und recycelt werden. (-> kein 'reserve' notwendig)
// noch weiter verbesserbar, wenn Container in einem Array vorliegen (Cache-lokalität).
template <typename D>
requires DataScheme<D>
class ThreadDataTransmitter {
    using Container = std::vector<D>;
    uint CONTAINER_SIZE = 8;
    static constexpr uint QUEUE_SIZE = 16;
    Container *current;
    spsc_queue<Container* , boost::lockfree::capacity<QUEUE_SIZE>> queue;
    
    inline void push() {
        while (!queue.push(std::move(current))) {
            ERR_PRINT("Zu viele Pakete im WorkerThread");
            //TODO: nutze binary_semaphore
            std::this_thread::sleep_for(VG::WORKER_THREAD_WAIT_TIME); // warten, falls Queue voll
        }
    }
    inline bool container_full() const {
        return CONTAINER_SIZE <= current->size();
    }
public:
    ThreadDataTransmitter() : current(new Container()) { current->reserve(CONTAINER_SIZE); }
    
    inline void queue_and_transmit_overflowing_task(D&& data) {
        if (container_full()) {
            // ERR_PRINT_ED(("Paketkapazität erreicht: " + std::to_string(data_transmitter.current->size())).c_str());
            transmit_queued_tasks();
        }
        current->emplace_back(data);
    }

    void transmit_queued_tasks() {
        push();
        current = new Container(); // new_delete Container
        current->reserve(CONTAINER_SIZE);
    }
    
    void consume_all_transmitted_tasks() {
        queue.consume_all([&](Container *container_ptr) {
            for (D &data : *container_ptr) {
                data.add_to_commit();
            }
            D::commit();
            delete container_ptr; // new_delete Container
        });
    }
};





template <typename Data>
class WeakThreadData;

template <typename Data>
class StrongThreadData {
    friend class WeakThreadData<Data>;

    std::unique_ptr<Data> data;
    
public:
    StrongThreadData() : data(nullptr) {}
    explicit StrongThreadData(Data &&d) : data(std::make_unique<Data>(d)) {}
    StrongThreadData(StrongThreadData &std) = delete;

    inline void absorb(WeakThreadData<Data> &other) {
        data = std::move(other.data);
    }

    inline void dismiss(WeakThreadData<Data> &other) {
        other.data = std::move(data);
    }
    
    inline Data* operator->() {
        return data.get();
    }
    inline Data& operator*() {
        return *data;
    }
    inline bool operator!() {
        return !data;
    }
    inline operator bool() {
        return data;
    }
};

template <typename Data>
class WeakThreadData {
    friend class StrongThreadData<Data>;

    std::unique_ptr<Data> data;

public:
    WeakThreadData() : data(std::make_unique<Data>()) {}
    explicit WeakThreadData(Data &&d) : data(std::make_unique<Data>(d)) {}
    WeakThreadData(WeakThreadData &std) = delete;

    inline Data* operator->() {
        return data.get();
    }
    inline Data& operator*() {
        return *data.get();
    }
    inline bool operator!() {
        return !data;
    }
    inline operator bool() {
        return data;
    }
};

class AllowChange {
    friend class ThreadPool;
private:
    AllowChange() {}
};

template <typename Data>
class SharedThreadData {
    //TODO: data_is_thread_save ist nicht fertig, aber ich brauche 'SharedThreadData' noch nicht..
    std::unique_ptr<Data> data;

public:
    SharedThreadData() : data(std::make_unique<Data>()) {}
    explicit SharedThreadData(Data &&d) : data(std::make_unique<Data>(d)) {}
    SharedThreadData(SharedThreadData &std) = delete;

    inline const Data* const operator->() const {
        return data.get();
    }
    inline const Data& operator*() const {
        return *data.get();
    }
    inline bool operator!() const {
        return !data;
    }
    inline operator bool() const {
        return data;
    }
    inline const Data& read_only() {
        return &data.get();
    }

    inline std::unique_ptr<Data> &get_data(const AllowChange& _) {
        return data;
    }
};

}; //namespace VG

using VG::StrongThreadData;
using VG::WeakThreadData;

#endif // SHARED_THREAD_DATA_H
