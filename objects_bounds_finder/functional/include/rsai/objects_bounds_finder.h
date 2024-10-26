#pragma once

#include <string>
#include <Eigen/Dense>

#include "common/promt_functions.hpp"
#include "common/progress_functions.hpp"

namespace rsai
{
    class objects_bounds_finder
    {
    public:
        template < class PromtFunc, class ProgressFunc >
        objects_bounds_finder (
                                gdal::shared_dataset &ds_vector
                                , gdal::shared_dataset &ds_raster
                                , gdal::shared_dataset &ds_meta
                                , gdal::shared_dataset &ds_out
                                , const int mask_size
                                , const int max_projection_length
                                , const bool force_rewrite
                                , const PromtFunc &promt_func = rewrite_layer_promt_dummy
                                , const ProgressFunc &progress_func = progress_dummy
                              );
    }; // class objects_bounds_finder
}; // namespace rsai

#include "rsai/objects_bounds_finder.hpp"
