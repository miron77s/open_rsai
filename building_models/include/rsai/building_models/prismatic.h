#pragma once

#include "rsai/building_models/abstract.h"

#include <utility>

namespace rsai
{
    namespace building_models
    {
        class prismatic:
                public abstract
        {
        public:
            prismatic () = default;
            prismatic ( const OGRLinearRing *object, const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step );
            prismatic ( gdal::polygon object, const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step );
            prismatic ( const prismatic &src );
            //prismatic ( prismatic &&src ) = default;

            virtual gdal::multipolygons     generate ( gdal::polygon object, const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step, const int vector_len ) override;
            virtual void                    generate ( const int vector_len ) override;
            virtual gdal::multipolygons     get      () const override;
            virtual void                    transform_2_raster ( const Eigen::Matrix3d &transform, const Eigen::Vector2d &tile_shift ) override;
            virtual void                    transform_2_world ( const Eigen::Matrix3d &transform, const Eigen::Vector2d &tile_shift ) override;
            virtual estimates               estimate ( const cv::Mat &tile, const double segmentize_step, double &memory_factor
                                                       , const std::vector < cv::Mat > &segments = {}, const double proj_sigma = 3.0
                                                       , const double shade_sigma = 3.0 ) const override;

            virtual estimates               estimate ( gdal::multipolygons structure, const Eigen::Matrix3d &world_2_raster, const Eigen::Vector2d &shift
                                                       , const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step
                                                       , const cv::Mat &tile, const double segmentize_step, double &memory_factor
                                                       , const std::vector < cv::Mat > &segments = {}, const double proj_sigma = 3.0
                                                       , const double shade_sigma = 3.0 ) const override;

            gdal::multipolygon       projection () const;
            gdal::multipolygon       shade () const;
            gdal::multipolygon       roof () const;

            prismatic                & operator = ( const prismatic &src );

        private:
            gdal::multipolygon       m_projection;
            gdal::multipolygon       m_shade;
            gdal::multipolygon       m_roof;

            Eigen::Vector2d          m_proj_pixel;
            Eigen::Vector2d          m_shade_pixel;

            double                  __line_polar_distance ( const Eigen::Vector2d &p1, const Eigen::Vector2d &p2 ) const;
            bool                    __render_polygon_2_mat ( OGRPolygon* polygon, cv::Mat& image, uint8_t color ) const;
        };
    }; // namespace building_models
}; // namespace rsai
