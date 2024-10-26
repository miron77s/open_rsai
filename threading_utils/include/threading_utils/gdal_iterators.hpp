#pragma once

#include "threading_utils/gdal_iterators.h"

#include <atomic>

template < class WorkerFunc, class ProgressFunc >
bool threading::dataset_iterator::operator () ( WorkerFunc worker, const ProgressFunc &progress_func, int numThreads ) const
{
    if ( !m_objects )
        return false;

    const int layers = m_objects->GetLayerCount ();

    bool result = true;
    for( int i = 0; i < layers; ++i )
    {
        auto layer = m_objects->GetLayer ( i );
        layer_iterator a_layer_iterator ( layer );
        result &= a_layer_iterator ( worker, i, layers, progress_func, numThreads );
    }

    return result;
}

template < class WorkerFunc, class ProgressFunc >
bool threading::layer_iterator::operator () ( WorkerFunc worker, const int layer_index, const int layers_count
                                               , const ProgressFunc &progress_func, int numThreads ) const
{
    if ( !m_layer )
        return false;

    std::atomic_int features_processed ( 0 );
    const float features_count = m_layer->GetFeatureCount ();

    threading::shared_layer_iterator source_layer_iter ( m_layer );
    {
        threading::worker_pool workers ( [&] ()
        {
            while ( auto feature = source_layer_iter.next_feature () )
            {
                const int current_feature_id = ++features_processed;
                progress_func ( layer_index + 1, layers_count, current_feature_id / features_count );

                worker ( feature, current_feature_id );
            }
        }, numThreads );
    }

    progress_func ( layer_index + 1, layers_count, 1.0f, true );

    return true;
}
