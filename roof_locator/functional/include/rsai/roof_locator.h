#pragma once

#include <string>
#include <Eigen/Dense>

#include "common/promt_functions.hpp"
#include "common/progress_functions.hpp"
#include "rsai/projection_and_shade_locator.h"

namespace rsai
{
    class roof_locator
    {
    public:
        template < class PromtFunc, class ProgressFunc >
        roof_locator  (
                        gdal::shared_dataset &ds_vector
                        , gdal::shared_dataset &ds_raster
                        , gdal::shared_dataset &ds_out
                        , const double segmentize_step
                        , const int projection_step
                        , const double roof_position_walk
                        , const int roof_variants
                        , const double min_first_pos_weight
                        , const double max_first_pos_deviation
                        , const bool force_rewrite
                        , const bool use_sam = false
                        , run_mode mode = run_mode::automatic
                        , interaction_mode interaction = interaction_mode::internal
                        , const PromtFunc &promt_func = rewrite_layer_promt_dummy
                        , const ProgressFunc &progress_func = progress_dummy
                      );
    }; // class projection_and_shade_locator
}; // namespace rsai

#include "rsai/roof_locator.hpp"
