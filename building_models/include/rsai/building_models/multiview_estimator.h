#pragma once

#include <vector>
#include <map>
#include "structure_estimator.h"
#include "prismatic.h"

namespace rsai
{
    namespace building_models
    {
        struct structure_response
        {
            int length = -1;
            double response = -1e+10;
            double shade_weight = 0.0;
            std::vector < gdal::multipolygons > projections_and_shades;

            structure_response ( int l, double r, double s, const std::vector < gdal::multipolygons > &projes_shades )
                : length ( l ), response ( r ), shade_weight ( s ), projections_and_shades ( projes_shades ) {}

            bool operator > ( const structure_response &rh ) const { return response > rh.response; }
        };

        using structure_responses = std::vector < structure_response >;

        class multiview_estimator
        {
        public:
            multiview_estimator ( const std::vector < gdal::polygon > &roofs, const std::vector < Eigen::Vector2d > &proj_steps
                                  , const std::vector < Eigen::Vector2d > &shade_steps
                                  , const std::vector < Eigen::Matrix3d > &world_2_rasters
                                  , const int from, const int to, const gdal::polygons &polygons = {} );

            structure_responses operator ()  ( const std::vector < Eigen::Vector2d > &tile_shifts
                                               , const std::vector < cv::Mat > &tiles, const double segmentize_step
                                               , const int responses_max ) const;

        private:
            multiview m_multiview_model;
        };
    }
};
