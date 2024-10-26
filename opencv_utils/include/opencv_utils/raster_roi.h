#pragma once

#include "gdal_utils/shared_dataset.h"
#include "gdal_utils/shared_geometry.h"
#include "eigen_utils/geometry.h"
#include <opencv2/opencv.hpp>

namespace opencv
{
    class dataset_roi_extractor
    {
    public:
        dataset_roi_extractor() = default;
        dataset_roi_extractor( const dataset_roi_extractor & src);
        dataset_roi_extractor( dataset_roi_extractor && src);
        dataset_roi_extractor(gdal::shared_dataset dataset);
        cv::Mat         roi     (gdal::bbox &bbox) const;
        gdal::bbox      raster_bbox    (gdal::polygon roi);

        Eigen::Matrix3d raster_2_world () const;
        Eigen::Matrix3d world_2_raster () const;

    private:
        gdal::shared_dataset m_dataset;
        mutable std::mutex m_mutex;
        Eigen::Matrix3d m_raster_2_world;
        Eigen::Matrix3d m_world_2_raster;

        void __set_transforms ();
    };
}; // namespace opencv
