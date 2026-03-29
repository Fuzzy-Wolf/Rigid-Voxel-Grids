#include "thread_pool.h"
#include "tasks.h"
#include "threads.h"
#include <atomic>


VG::ThreadPool* const VG::ThreadPool::singleton = new ThreadPool();
std::atomic_bool VG::ThreadPool::started{false};

uint VG::ThreadPool::get_number_of_threads() {return ThreadCounter::get_count();}

VG::ThreadPool::ThreadPool() {}

void VG::ThreadPool::start() {
    ERR_FAIL_COND_MSG(started.load(std::memory_order_acquire), "ThreadPool mehrere male gestartet!");
    started.store(true,std::memory_order_release);

    threads.reserve(4);

    threads.emplace_back(new WorkerThread<LoadingTask>{});
    threads.emplace_back(new WorkerThread<MeshingTask>{});
    threads.emplace_back(new WorkerThread<CharaktersPhysicsTask>());
}
