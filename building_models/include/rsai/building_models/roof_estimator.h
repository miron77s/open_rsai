#pragma once

#include <vector>
#include <opencv2/opencv.hpp>
#include <Eigen/Dense>
#include "eigen_utils/geometry.h"
#include "gdal_utils/shared_geometry.h"

#define RENDER_SAM_MASKS

namespace rsai
{
    namespace building_models
    {
        struct roof_response
        {
            gdal::polygon aligned_roof;
            gdal::polygon aligned_roof_in_tile;
            Eigen::Vector2d shift_world;
            Eigen::Vector2d shift_on_tile;
            double value = -1e+300;
            double deviation = 0.0;

            bool operator > ( const roof_response &rh ) const { return value > rh.value; }
        };

        using roof_responses = std::vector < roof_response >;

        class roof_estimator
        {
        public:
            roof_estimator ( const float position_walk, const Eigen::Matrix3d &world_2_raster, const Eigen::Vector2d &proj_world
                             , const Eigen::Vector2d &shade_world, const double max_length, const cv::Mat &tile_gray, const cv::Mat &mask = {}  );

            roof_responses  operator () ( const gdal::polygon &roof, const Eigen::Vector2d &tile_offset, const int roof_variants );
            roof_responses  operator () ( const gdal::polygon &roof, const Eigen::Vector2d &tile_offset
                                          , const gdal::polygons &segments, const int roof_variants, const std::string dst_dir = "" );

            cv::Mat         heatmap () const;

        private:
            gdal::bbox              m_search_bbox;
            gdal::polygon    m_search_region;
            Eigen::Matrix3d         m_world_2_raster;
            cv::Mat                 m_edges;
            cv::Mat                 m_mask;
            cv::Mat                 m_heatmap;
            Eigen::Vector2d         m_proj_pixel;
            Eigen::Vector2d         m_shade_pixel;
            const double            m_max_length;
            const float             m_position_walk;

            static constexpr int response_calculation_step = 1;
            static constexpr int non_maxima_suppression_half_width = 3;

            struct xy
            {
                int x = 0, y = 0;
                bool operator < ( const xy &rh ) const;
            };

            gdal::polygon __create_search_region ( const float position_walk );
            roof_responses __heatmap_non_maxima_suppression ( cv::Mat heatmap, const double start_x, const double start_y );
            void __fill_responses ( roof_responses &responses, gdal::polygon roof_geometry, const Eigen::Matrix3d &world_2_raster
                                    , const Eigen::Vector2d &tile_offset, const int roof_variants
                                    , std::map < xy, double > &distance_weight_mapping ) const;
        };
    };
};
