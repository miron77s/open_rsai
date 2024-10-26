#pragma once

#include <string>
#include <Eigen/Dense>

#include "common/promt_functions.hpp"
#include "common/progress_functions.hpp"
#include "gdal_utils/shared_dataset.h"

namespace rsai
{
    class segany_markup_by_objects
    {
    public:
        template < class PromtFunc, class LayerPromtFunc, class ProgressFunc >
        segany_markup_by_objects  (   gdal::shared_dataset roof_vector
                                    , gdal::shared_dataset raster
                                    , gdal::shared_dataset markup_map
                                    , const std::string &markup_directory
                                    , int tile_buffer_size
                                    , const bool force_rewrite
                                    , const PromtFunc &promt_func = rewrite_directory_promt_dummy
                                    , const LayerPromtFunc &layer_promt_func = rewrite_layer_promt_dummy
                                    , const ProgressFunc &progress_func = progress_dummy
                                  );
    }; // class segany_markup_by_objects
}; // namespace rsai

#include "rsai/segany_markup_by_objects.hpp"
