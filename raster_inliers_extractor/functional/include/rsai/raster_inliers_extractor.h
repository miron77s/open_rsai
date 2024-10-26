#pragma once

#include <string>

#include "common/promt_functions.hpp"
#include "common/progress_functions.hpp"

#include "eigen_utils/gdal_bridges.h"
#include "eigen_utils/geometry.h"

namespace rsai
{

    class raster_inliers_extractor
    {
    public:
        template < class PromtFunc, class ProgressFunc >
        raster_inliers_extractor ( gdal::shared_dataset &ds_vector
                                   , gdal::shared_dataset &ds_raster
                                   , gdal::shared_dataset &ds_out
                                   , gdal::shared_dataset &ds_roi
                                   , const bool force_rewrite
                                   , const bool crop_geometry
                                   , const std::string &semanic_filter
                                   , const int min_raster_obj_size
                                   , const PromtFunc &promt_func = rewrite_layer_promt_dummy
                                   , const ProgressFunc &progress_func = console_progress
                                 );

    private:
        template < class GeometryType >
        bool __check_raster_size ( std::shared_ptr < GeometryType > geometry, const Eigen::Matrix3d &world_2_raster, const int min_raster_obj_size );

        template < class GeometryType >
        bool __check_raster_size ( GeometryType * geometry, const Eigen::Matrix3d &world_2_raster, const int min_raster_obj_size );

    }; // class raster_inliers_extractor

}; // namespace rsai

#include "rsai/raster_inliers_extractor.hpp"
