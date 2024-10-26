#pragma once

#include <string>
#include <opencv2/opencv.hpp>
#include "rsai/building_models/structure_estimator.h"
#include "rsai/building_models/roof_estimator.h"
#include "rsai/building_models/multiview_estimator.h"

namespace rsai
{
    class building_variants_saver
    {
    public:
        building_variants_saver ( const std::string &target_dir );

        bool write ( const std::string id, const building_models::structures &positions, const cv::Mat &tile
                     , const Eigen::Vector2d & tile_top_left, const Eigen::Matrix3d &raster_2_world );
        bool write ( const std::string id, gdal::polygon roof_geometry, const building_models::roof_responses &positions, const cv::Mat &tile
                     , const Eigen::Vector2d & tile_top_left, const Eigen::Matrix3d &raster_2_world );

        bool write ( const std::string id, std::vector < gdal::polygon > roofs, const building_models::structure_responses &structures
                     , const std::vector < cv::Mat > &tiles, const std::vector < Eigen::Vector2d > tile_shifts
                     , const std::vector < Eigen::Matrix3d > &raster_2_worlds, const std::vector < std::string > &save_dirs );

    private:
        std::string m_target_dir;        
        bool __write_wkt ( const std::string &file_name, const OGRGeometry * geom ) const;
    }; // class buiding_variants_saver
}; // namespace rsai
