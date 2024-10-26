#pragma once

#include <string>
#include <Eigen/Dense>

#include "common/promt_functions.hpp"
#include "common/progress_functions.hpp"
#include "rsai/projection_and_shade_locator.h"

namespace rsai
{
    class multiview_building_reconstructor
    {
    public:
        template < class PromtFunc, class ProgressFunc >
        multiview_building_reconstructor  (
                                          gdal::shared_datasets &ds_vectors
                                        , gdal::shared_datasets &ds_rasters
                                        , gdal::shared_datasets &ds_bounds
                                        , gdal::shared_datasets &ds_outs
                                        , const double segmentize_step
                                        , const int projection_step
                                        , const double roof_position_walk
                                        , const int roof_variants
                                        , const int shade_variants
                                        , const bool force_rewrite
                                        , const bool use_sam = false
                                        , run_mode mode = run_mode::automatic
                                        , interaction_mode interaction = interaction_mode::internal
                                        , const PromtFunc &promt_func = rewrite_layer_promt_dummy
                                        , const ProgressFunc &progress_func = progress_dummy
                                      );
    }; // class multiview_building_reconstructor
}; // namespace rsai

#include "rsai/multiview_building_reconstructor.hpp"
