#pragma once

#include <string>

#include "common/promt_functions.hpp"
#include "common/progress_functions.hpp"

#include "eigen_utils/gdal_bridges.h"
#include "eigen_utils/geometry.h"

namespace rsai
{

    class map_updater
    {
    public:
        template < class PromtFunc, class ProgressFunc >
        map_updater (
                                 gdal::shared_dataset &ds_vector
                               , gdal::shared_dataset &ds_updating
                               , gdal::shared_dataset &ds_roi
                               , gdal::shared_dataset &ds_out
                               , const double iou_match_thresh
                               , const double iou_min_thresh
                               , const bool save_difference
                               , const bool save_updated
                               , const PromtFunc &promt_func = rewrite_layer_promt_dummy
                               , const ProgressFunc &progress_func = console_progress
                             );

    }; // class map_updater

}; // namespace rsai

#include "rsai/map_updater.hpp"
