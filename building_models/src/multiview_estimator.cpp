#include "rsai/building_models/multiview_estimator.h"

using namespace rsai::building_models;

rsai::building_models::multiview_estimator::multiview_estimator ( const std::vector < gdal::polygon > &roofs, const std::vector < Eigen::Vector2d > &proj_steps
                                                                  , const std::vector < Eigen::Vector2d > &shade_steps
                                                                  , const std::vector < Eigen::Matrix3d > &world_2_rasters
                                                                  , const int from, const int to, const gdal::polygons &polygons )
    : m_multiview_model ( roofs, proj_steps, shade_steps, world_2_rasters, from, to, prismatic () )
{

}

structure_responses rsai::building_models::multiview_estimator::operator ()  ( const std::vector < Eigen::Vector2d > &tile_shifts
                                                                             , const std::vector < cv::Mat > &tiles, const double segmentize_step
                                                                             , const int responses_max ) const
{
    structure_responses responses;

    auto first_response = 0.0;
    for ( int length = m_multiview_model.range_from(); length < m_multiview_model.range_to(); ++length )
    {
        double shade_weight = 0.0;
        double response = 0.0;
        if ( length == 1 )
        {
            response = 1e+30;
            first_response = m_multiview_model.estimate( length, tile_shifts, tiles, segmentize_step, shade_weight );
        }
        else
            response = m_multiview_model.estimate( length, tile_shifts, tiles, segmentize_step, shade_weight );

        auto geometries = m_multiview_model.get ( length );
        responses.emplace_back ( length, response, shade_weight, std::move ( geometries ) );
    }

    std::sort ( responses.begin(), responses.end(), std::greater < structure_response > () );
    responses [0].response = first_response;

    structure_responses out;
    for ( int i = 0; i < responses.size() && i < responses_max; ++i )
        out.push_back ( responses [i] );

    return std::move ( out );
}
