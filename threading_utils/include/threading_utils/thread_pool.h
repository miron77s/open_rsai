#pragma once

#include <vector>
#include <thread>
#include "thread_safe_feature_layer.h"

namespace threading
{
    class worker_pool
    {
    public:
        template < class WorkerFunc >
        worker_pool(WorkerFunc worker, int numThreads = std::thread::hardware_concurrency() - 1);
        ~worker_pool();

    private:
        std::vector<std::thread> workers;
    };
}; // namespace threading

template < class WorkerFunc >
threading::worker_pool::worker_pool(WorkerFunc worker, int numThreads)
{
    for(int i = 0; i < numThreads; ++i)
    {
        workers.emplace_back(worker);
    }
}
