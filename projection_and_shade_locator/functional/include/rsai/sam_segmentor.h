#pragma once

#include <string>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <Eigen/Dense>
#include "gdal_utils/shared_feature.h"
#include "gdal_utils/shared_dataset.h"
#include "gdal_utils/shared_geometry.h"
#include "eigen_utils/geometry.h"

namespace rsai
{
    class sam_tile
    {
    public:
        sam_tile ( gdal::shared_dataset raster, const std::string &path, const int index, const Eigen::Vector2d &pos, const Eigen::Vector2d &size );

        gdal::bbox  bbox () const;
        bool        verify ( const gdal::bbox &tile_bbox ) const;
        double      distance ( const gdal::bbox &tile_bbox ) const;
        //cv::Mat   segments ( const gdal::bbox &tile_bbox );
        gdal::polygons segments ( const gdal::bbox &tile_bbox );

    private:
        const int               m_index;
        Eigen::Vector2d         m_pos;
        Eigen::Vector2d         m_size;
        Eigen::Vector2d         m_center;
        const std::string       m_file_name;
        const std::string       m_segs_name;
    };

    class sam_tiles
    {
    public:
        sam_tiles ( gdal::shared_dataset raster, const std::string &path );
        sam_tiles ( gdal::shared_dataset raster, const std::string &path, gdal::geometry coverage, const Eigen::Vector2d tile_size, const Eigen::Vector2d tile_step );

        bool verify_or_add ( const gdal::bbox &tile_bbox );
        //cv::Mat segments ( const gdal::bbox &tile_bbox );
        gdal::polygons segments ( const gdal::bbox &tile_bbox );

    private:
        std::vector < sam_tile >    m_tiles;
        gdal::shared_dataset        m_raster;
        const std::string           m_path;
        int                         m_index;
        std::mutex                  m_lock;
    };

    class sam_segmentor
    {
    public:
        sam_segmentor ( gdal::shared_dataset raster, gdal::shared_dataset objects_vector, const std::string & );
        ~sam_segmentor ();

        template < class ProgressFunc >
        bool operator () ( const ProgressFunc &progress_func );

    private:
        gdal::shared_dataset    m_raster;
        gdal::shared_dataset    m_objects_vector;
        const std::string       m_store_dir;
    };
}

#include "rsai/sam_segmentor.hpp"
