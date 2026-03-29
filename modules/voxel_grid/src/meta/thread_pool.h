#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>


#include "threads.h"
#include "shared_data.h"


namespace VG {
/*
class MainThreadStorage {
public:
    std::vector<UpdateData> changing_chunks;
    MainThreadStorage() {
        changing_chunks.reserve(20);
    }
};
*/
class ThreadPool {
    static ThreadPool* const singleton;
    static std::atomic_bool started;

public:
    
    /*
    MainThreadStorage main_thread_storage;
    */
    static uint get_number_of_threads();

    std::vector<Thread*> threads;

    
    template<typename Threadtype>
    static void inline wait_for_thread(Threadtype& t) {
        while (!t.is_ready_and_waiting()) {
        }
    }

    static inline const AllowChange wait_for_all_threads() {
        for (auto &thread : singleton->threads) {
            while (!thread->is_ready_and_waiting()) {
            }
        }
        return AllowChange();
    }

    //TODO: in GD-Script einstellbar, welche Threads ich haben möchte
    void start();

    inline static ThreadPool* get_singleton() {return singleton;}
    ThreadPool();
    ~ThreadPool() {
        for (auto& thread : threads) {
            delete thread;
        };
    }
};

}; //namespace VVG

using VG::ThreadPool;

#endif // THREAD_POOL_H