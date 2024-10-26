#pragma once

#include <string>

#include "common/promt_functions.hpp"
#include "common/progress_functions.hpp"

#include "eigen_utils/gdal_bridges.h"
#include "eigen_utils/geometry.h"

namespace rsai
{

    class vector_iou_estimator
    {
    public:
        template < class PromtFunc, class ProgressFunc >
        vector_iou_estimator (
                                 gdal::shared_datasets &ds_vectors
                               , gdal::shared_datasets &ds_ground_trues
                               , gdal::shared_datasets &ds_outs
                               , const PromtFunc &promt_func = rewrite_layer_promt_dummy
                               , const ProgressFunc &progress_func = console_progress
                             );

    }; // class vector_iou_estimator

}; // namespace rsai

#include "rsai/vector_iou_estimator.hpp"
