#pragma once

#include "common/progress_functions.hpp"
#include "gdal_utils/shared_dataset.h"
#include "threading_utils/thread_pool.h"

namespace threading
{
    class dataset_iterator
    {
    public:

        dataset_iterator ( gdal::shared_dataset objects ): m_objects ( objects ) {}

        template < class WorkerFunc, class ProgressFunc >
        bool operator () ( WorkerFunc worker, const ProgressFunc &progress_func = progress_dummy
                           , int numThreads = std::thread::hardware_concurrency() - 1 ) const;

    private:
        gdal::shared_dataset m_objects;
    };

    class layer_iterator
    {
    public:
        layer_iterator ( gdal::ogr_layer *layer ): m_layer ( layer ) {}

        template < class WorkerFunc, class ProgressFunc >
        bool operator () ( WorkerFunc worker, const int layer_index, const int layers_count
                           , const ProgressFunc &progress_func = progress_dummy
                           , int numThreads = std::thread::hardware_concurrency() - 1 ) const;

    private:
        gdal::ogr_layer *m_layer;
    };
}

#include "threading_utils/gdal_iterators.hpp"
