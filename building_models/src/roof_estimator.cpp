#include "rsai/building_models/roof_estimator.h"

#include "gdal_utils/operations.h"
#include "eigen_utils/math.hpp"
#include "eigen_utils/geometry.h"
#include "opencv_utils/gdal_bridges.h"
#include "opencv_utils/geometry_renderer.h"
#include "differentiation/gauss_directed_derivative.h"
#include "differentiation/convolution_mask.h"

using namespace gdal;
using namespace rsai::building_models;

rsai::building_models::roof_estimator::roof_estimator( const float position_walk, const Eigen::Matrix3d &world_2_raster, const Eigen::Vector2d &proj_world
                                                       , const Eigen::Vector2d &shade_world, const double max_length, const cv::Mat &tile_gray, const cv::Mat &mask ):
    m_world_2_raster ( world_2_raster ), m_mask ( mask ), m_max_length ( max_length ), m_position_walk ( position_walk )
{
    const Eigen::Matrix2d transform     = world_2_raster.block < 2, 2 > ( 0, 0 );
    m_proj_pixel    = transform * proj_world;
    m_shade_pixel   = transform * shade_world;

    m_search_region = __create_search_region ( position_walk );
    m_search_bbox = gdal::bbox ( m_search_region );

    const auto shade_angle = eigen::to_polar ( m_shade_pixel ) [1];
    convolution_mask edges_mask ( gauss::first_directed_derivative ( shade_angle, 3.0, 0.5 ), 9, 9 );

    auto convolution = edges_mask.conv < uint8_t, int32_t > ( tile_gray, CV_32S, m_mask );

    double minVal, maxVal;
    cv::minMaxLoc(convolution, &minVal, &maxVal); //find minimum and maximum intensities

    convolution.convertTo(m_edges, CV_8U, 255.0/maxVal, 0);
}

rsai::building_models::roof_responses rsai::building_models::roof_estimator::operator () ( const gdal::polygon &roof, const Eigen::Vector2d &tile_offset
                                                                                           , const int roof_variants )
{
    auto roof_local = gdal::operator * ( roof, m_world_2_raster );
    roof_local = gdal::operator +( roof_local, tile_offset );
    roof_local->segmentize ( 1.0 );

    const auto roof_maximum_walk = std::sqrt ( roof->get_Area() );

    auto search_bbox    = m_search_bbox;
    auto search_region  = m_search_region;
    if ( roof_maximum_walk < m_position_walk )
    {
        search_region = __create_search_region ( roof_maximum_walk );
        search_bbox = gdal::bbox ( m_search_region );
    }

    opencv::polygon_bounded_ops estimator ( m_edges, roof_local );

    cv::Mat heatmap = cv::Mat::zeros ( m_edges.size (), CV_32F );


    const auto start_x = roof_local->getExteriorRing()->getX( 0 ),
               start_y = roof_local->getExteriorRing()->getY( 0 );

    std::map < xy, double > distance_weight_mapping;
    for ( int y = search_bbox.top_left().y (); y <= search_bbox.bottom_right().y (); y += response_calculation_step )
    {
        for ( int x = search_bbox.top_left().x (); x <= search_bbox.bottom_right().x (); x += response_calculation_step )
        {
            OGRPoint pt ( x, y );
            if ( search_region->Contains ( &pt ) )
            {
                const Eigen::Vector2d curr_shift ( x, y );
                distance_weight_mapping [{ x, y }] = curr_shift.norm() / roof_maximum_walk;

                auto response = estimator.nonzero_weighted_sum ( x, y );
                heatmap.at < float > ( y + start_y, x + start_x ) = response;
            }
        }
    }

    auto responses = __heatmap_non_maxima_suppression ( heatmap, start_x, start_y );
    __fill_responses ( responses, roof, m_world_2_raster, tile_offset, roof_variants, distance_weight_mapping );

    return std::move ( responses );
}

rsai::building_models::roof_responses rsai::building_models::roof_estimator::operator () ( const gdal::polygon &roof, const Eigen::Vector2d &tile_offset
                                                                                           , const gdal::polygons &segments, const int roof_variants, const std::string dst_dir )
{
    auto prepare_start = std::chrono::high_resolution_clock::now();

    auto roof_local = gdal::operator * ( roof, m_world_2_raster );
    roof_local = gdal::operator +( roof_local, tile_offset );
    roof_local->segmentize ( 1.0 );

    const double sigma = 2;
    const int mask_half = std::round ( sigma * 3.0 );
    convolution_mask mask ( gauss::function ( sigma ), mask_half, mask_half );

    const auto roof_maximum_walk = std::sqrt ( roof->get_Area() );
//    const auto roof_area_pixel = roof_local->get_Area();

    auto search_bbox    = m_search_bbox;
    auto search_region  = m_search_region;
    if ( roof_maximum_walk < m_position_walk )
    {
        search_region = __create_search_region ( roof_maximum_walk );
        search_bbox = gdal::bbox ( search_region );
    }

    //opencv::geometry_renderer all_renderer ( m_edges.size () );
    std::vector < cv::Mat > segment_maps ( segments.size() );

    for ( int i = 0; i < segments.size(); ++i )
    {
        const auto & segment = segments [i];
//        const auto segment_area_pixel = segment->get_Area();

//        const auto segment_2_roof_ratio = segment_area_pixel / roof_area_pixel;
//        if ( segment_2_roof_ratio < 0.35 || segment_2_roof_ratio > 3.0 )
//            continue;

        auto & segment_map = segment_maps [i];
        opencv::geometry_renderer renderer ( m_edges.size () );

        renderer ( segment, mask, opencv::blender_max () );

        cv::Mat image = renderer.image();

        //all_renderer( image, opencv::blender_max () );

        double minVal, maxVal;
        cv::minMaxLoc(image, &minVal, &maxVal);

        image.convertTo(image, CV_8U, 255.0/maxVal, 0);

        segment_maps [i] = image;

#       if defined ( RENDER_SAM_MASKS )
        //cv::imwrite ( dst_dir + std::to_string ( i ) + ".jpg", image );
#       endif
    }

    //cv::Mat image = all_renderer.image();
    double minVal, maxVal;
    //cv::minMaxLoc(image, &minVal, &maxVal);

    //image.convertTo(image, CV_8U, 255.0/maxVal, 0);

#   if defined ( RENDER_SAM_MASKS )    
    cv::imwrite ( dst_dir + "segms.png", m_edges );
#   endif

    auto prepare_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> prepare_elapsed = prepare_stop - prepare_start;
    //std::cout << "prepare " << prepare_elapsed.count() << " s\n";

    auto calc_start = std::chrono::high_resolution_clock::now();

    cv::Mat heatmap = cv::Mat::zeros ( m_edges.size (), CV_32F );

    const auto start_x = roof_local->getExteriorRing()->getX( 0 ),
               start_y = roof_local->getExteriorRing()->getY( 0 );

    std::vector < opencv::polygon_bounded_ops > estimators;
    estimators.reserve ( segments.size() );
    for ( int i = 0; i < segments.size(); ++i )
        estimators.emplace_back ( segment_maps [i], roof_local );

    std::map < xy, double > distance_weight_mapping;
    opencv::polygon_bounded_ops edge_estimator ( m_edges, roof_local );
    //opencv::polygon_bounded_ops total_estimator ( image, roof_local );

    for ( int y = search_bbox.top_left().y (); y <= search_bbox.bottom_right().y (); y += response_calculation_step )
    {
        for ( int x = search_bbox.top_left().x (); x <= search_bbox.bottom_right().x (); x += response_calculation_step )
        {
            OGRPoint pt ( x, y );
            if ( search_region->Contains ( &pt ) )
            {
                const Eigen::Vector2d curr_shift ( x, y );
                distance_weight_mapping [{ x, y }] = curr_shift.norm() / roof_maximum_walk;

                float max_response = 0.0f;

                //auto roof_shifted = roof_local + Eigen::Vector2d { x, y };
                for ( int i = 0; i < segments.size(); ++i )
                {
                    //const auto & segment = segments [i];
                    //auto &segment_map = segment_maps [i];

//                    if ( segment_map.empty() )
//                        continue;

//                    if ( !roof_shifted->Intersects ( segment.get () ) )
//                        continue;

                    //opencv::polygon_bounded_ops estimator ( segment_map, roof_local );
                    const float response = estimators [i].sum_unsafe ( x, y );

                    max_response = std::max ( max_response, response );
                }

                //const float total_response = total_estimator.sum_unsafe ( x, y );
                const float edge_response = edge_estimator.sum_unsafe ( x, y );

                heatmap.at < float > ( y + start_y, x + start_x ) = max_response + /*total_response +*/ edge_response;
            }
        }
    }

    auto calc_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> calc_elapsed = calc_stop - calc_start;
    //std::cout << "calc " << calc_elapsed.count() << " s\n";

    auto post_proc_start = std::chrono::high_resolution_clock::now();

    cv::minMaxLoc(heatmap, &minVal, &maxVal);
    cv::Mat heat_out;
    heatmap.convertTo(heat_out, CV_8U, 255.0/maxVal, 0);
    cv::imwrite ( dst_dir + "heats.png", heat_out );

    auto responses = __heatmap_non_maxima_suppression ( heatmap, start_x, start_y );
    __fill_responses ( responses, roof, m_world_2_raster, tile_offset, roof_variants, distance_weight_mapping );

    auto post_proc_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> post_proc_elapsed = post_proc_stop - post_proc_start;
    //std::cout << "post proc " << post_proc_elapsed.count() << " s\n";

    return std::move ( responses );
}

cv::Mat rsai::building_models::roof_estimator::heatmap () const
{
    return m_heatmap;
}

gdal::polygon rsai::building_models::roof_estimator::__create_search_region ( const float position_walk )
{
    const Eigen::Matrix2d transform = m_world_2_raster.block < 2, 2 > ( 0, 0 );
    gdal::bbox search_bbox ( { -position_walk, -position_walk }, { position_walk, position_walk } );
    auto search_region_base = search_bbox.transform ( transform );

    auto proj_norm = std::max ( std::max ( std::abs ( m_proj_pixel.x() ), std::abs ( m_proj_pixel.y() ) )
                                , std::max ( std::abs ( m_shade_pixel.x() ), std::abs ( m_shade_pixel.y() ) ) );

    auto proj_step = m_proj_pixel / proj_norm;

    return gdal::project ( search_region_base, proj_step, m_max_length );
}

roof_responses rsai::building_models::roof_estimator::__heatmap_non_maxima_suppression ( cv::Mat heatmap, const double start_x, const double start_y )
{
    roof_responses responses;
    responses.reserve ( 100 );

    m_heatmap = cv::Mat::zeros ( m_edges.size (), CV_32F );
    for(int y = 0; y < m_heatmap.rows; ++y)
    {
        for(int x = 0; x < m_heatmap.cols; ++x)
        {
            bool isLocalMax = true;

            const int y0 = (y > non_maxima_suppression_half_width ? y - non_maxima_suppression_half_width : 0);
            const int x0 = (x > non_maxima_suppression_half_width ? x - non_maxima_suppression_half_width : 0);
            const int y1 = (y + non_maxima_suppression_half_width < m_heatmap.rows ? y + non_maxima_suppression_half_width : m_heatmap.rows - 1);
            const int x1 = (x + non_maxima_suppression_half_width < m_heatmap.cols ? x + non_maxima_suppression_half_width :  m_heatmap.cols - 1);

            const auto curr_value = heatmap.at<float>(y, x);

            if ( curr_value == 0.0f )
                continue;

            for(int yy = y0; yy <= y1; ++yy)
            {
                for(int xx = x0; xx <= x1; ++xx)
                {
                    if(heatmap.at<float>(yy, xx) > curr_value )
                    {
                        isLocalMax = false;
                        break;
                    }
                }

                if(!isLocalMax)
                    break;
            }

            if(isLocalMax)
            {
                const auto response = heatmap.at<float>(y, x);
                m_heatmap.at<float>(y, x) = response;
                responses.push_back ( { {}, {}, {}, { x - start_x, y - start_y }, response } );
            }
        }
    }

    double minVal, maxVal;
    cv::minMaxLoc(m_heatmap, &minVal, &maxVal);

    m_heatmap.convertTo(m_heatmap, CV_8U, 255.0/maxVal, 0);

    std::sort ( responses.begin(), responses.end(), std::greater < roof_response > () );
    return std::move ( responses );
}

void rsai::building_models::roof_estimator::__fill_responses ( roof_responses &responses, gdal::polygon roof_geometry
                                                               , const Eigen::Matrix3d &world_2_raster, const Eigen::Vector2d &tile_offset, const int roof_variants
                                                               , std::map < xy, double > &distance_weight_mapping ) const
{
    const Eigen::Matrix3d &raster_2_world = world_2_raster.inverse();

    if ( responses.size() > roof_variants )
    {
        responses.resize ( roof_variants );
        responses.shrink_to_fit();
    }

    for ( auto & response : responses )
    {
        const Eigen::Vector2i sh_pixel = response.shift_on_tile.cast < int > ();
        const Eigen::Vector2d roof_shift = raster_2_world.block < 2, 2 > ( 0, 0 ) * response.shift_on_tile;
        auto shifted_roof = gdal::operator + ( roof_geometry, roof_shift );

        auto shifted_roof_raster = gdal::operator * ( shifted_roof, world_2_raster );
        auto tiled_roof_raster = gdal::operator + ( shifted_roof_raster, tile_offset );

        response.shift_world = roof_shift;
        response.aligned_roof = shifted_roof;
        response.aligned_roof_in_tile = tiled_roof_raster;
        response.deviation = distance_weight_mapping [ { sh_pixel [0], sh_pixel [1] } ];
    }
}

bool rsai::building_models::roof_estimator::xy::operator < ( const xy &rh ) const
{
    return y < rh.y || ( y == rh.y && x < rh.x );
}
