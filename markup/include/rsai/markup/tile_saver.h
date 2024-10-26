#pragma once

#include <string>
#include <opencv2/opencv.hpp>
#include <Eigen/Dense>

#include <gdal_utils/shared_dataset.h>
#include <gdal_utils/shared_feature.h>
#include <gdal_utils/shared_geometry.h>
#include "eigen_utils/geometry.h"
#include "common/definitions.h"
#include "opencv_utils/raster_roi.h"

namespace rsai
{
    class tile_saver
    {
    public:
        tile_saver ( const std::string &path, gdal::shared_dataset raster );

        gdal::bbox operator () ( gdal::shared_feature feature, const int tile_buffer_size = 0 );

        gdal::bbox get_bbox ( gdal::shared_feature feature, const int tile_buffer_size = 0 ) const;

    private:
        const std::string               m_path;
        gdal::shared_dataset            m_raster;
        Eigen::Matrix3d                 m_world_2_raster;
        opencv::dataset_roi_extractor   m_ds_tile_extractor;
    };
};
