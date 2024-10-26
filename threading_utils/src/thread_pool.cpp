#include "threading_utils/thread_pool.h"

#include <iostream>

using namespace threading;

worker_pool::~worker_pool()
{
    for(auto& worker : workers)
    {
        worker.join();
    }
}
