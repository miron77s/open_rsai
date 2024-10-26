#pragma once

#include <vector>
#include <map>
#include "roof_estimator.h"
#include "prismatic.h"

namespace rsai
{
    namespace building_models
    {
        struct model_response
        {
            int length = -1;
            double response = -1e+10;
            prismatic model;

            model_response ( int l, double r, const prismatic &m ): length ( l ), response ( r ), model ( m ) {}

            bool operator > ( const model_response &rh ) const { return response > rh.response; }
        };

        using model_responses = std::vector < model_response >;

        struct structure
                : public roof_response
        {
            model_responses shades;
        };

        using structure_map = std::map < Eigen::Vector2d, structure >;
        using structures = std::vector < structure >;

        class structure_estimator
        {
        public:
            structure_estimator ( prismatic model, const roof_responses &responses
                                 , const cv::Mat &tile_gray, const Eigen::Vector2d &tile_tl_corner
                                 , const Eigen::Matrix3d &world_2_raster, const double segmentize_step
                                 , const gdal::polygons &polygons = {} );

            structures operator ()  ( const double max_length, const double projection_step
                                          , const int roof_responses_max, const int shade_responses_max ) const;

        private:
            mutable prismatic       m_model;
            const roof_responses  & m_responses;
            const cv::Mat         & m_tile_gray;
            const gdal::polygons    m_segments;
            const Eigen::Vector2d & m_tile_tl_corner;
            const Eigen::Matrix3d & m_world_2_raster;
            const double            m_segmentize_step;
        };
    };
};

namespace Eigen
{
    static bool operator < ( const Eigen::Vector2d &lh, const Eigen::Vector2d &rh ) { return lh.y () < rh.y () || lh.y () == rh.y () && lh.x () < rh.x (); }
};
