#include "rsai/building_models/prismatic.h"

#include <limits>
#include <iostream>

#include "eigen_utils/gdal_bridges.h"
#include "eigen_utils/geometry.h"
#include "eigen_utils/math.hpp"
#include "differentiation/convolution_mask.h"
#include "differentiation/gauss_directed_derivative.h"
#include "eigen_utils/geometry.h"

using namespace gdal;

rsai::building_models::prismatic::prismatic ( const OGRLinearRing *object, const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step )
    : abstract ( object, proj_step, shade_step )
{

}

rsai::building_models::prismatic::prismatic ( gdal::polygon object, const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step )
    : abstract ( object, proj_step, shade_step )
{

}

rsai::building_models::prismatic::prismatic ( const prismatic &src )
    : abstract ( src ), m_proj_pixel ( src.m_proj_pixel ), m_shade_pixel ( src.m_shade_pixel )
{
    m_projection = src.m_projection ? instance ( src.m_projection->clone () ) : nullptr;
    m_shade = src.m_shade ? instance ( src.m_shade->clone () ) : nullptr;
    m_roof = src.m_roof ? instance ( src.m_roof->clone () ) : nullptr;
}

gdal::multipolygons rsai::building_models::prismatic::generate ( gdal::polygon object, const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step, const int vector_len )
{
    auto base = object + ( -proj_step * vector_len );
    auto proj = gdal::project ( base, proj_step, vector_len );
    auto shade = gdal::project ( base, shade_step, vector_len );

    auto pure_shade = shade->Difference ( proj.get () );
    gdal::multipolygon shade_poly;
    if ( pure_shade != nullptr )
    {
        if ( wkbFlatten ( pure_shade->getGeometryType()) == wkbPolygon )
        {
            shade_poly = instance < multipolygon > ();
            shade_poly->addGeometryDirectly ( pure_shade );
        }
        else
        if ( wkbFlatten ( pure_shade->getGeometryType()) == wkbMultiPolygon )
            shade_poly = instance ( pure_shade->toMultiPolygon () );
    }

    auto roof_poly = instance < multipolygon > ();
    roof_poly->addGeometryDirectly ( object->clone () );

    auto pure_proj = proj->Difference ( roof_poly.get () );

    gdal::multipolygon projection_poly;
    if ( pure_proj != nullptr )
    {
        if ( wkbFlatten ( pure_proj->getGeometryType()) == wkbPolygon )
        {
            projection_poly = instance < multipolygon > ();
            projection_poly->addGeometryDirectly ( pure_proj );
        }
        else
        if ( wkbFlatten ( pure_proj->getGeometryType()) == wkbMultiPolygon )
            projection_poly = instance ( pure_proj->toMultiPolygon () );
    }

    return { roof_poly, projection_poly, shade_poly };
}


void rsai::building_models::prismatic::generate ( const int vector_len )
{
    auto structure = generate ( m_object, m_proj_step, m_shade_step, vector_len );

    m_roof = structure [0];
    m_projection = structure [1];
    m_shade = structure [2];
}

gdal::multipolygons rsai::building_models::prismatic::get () const
{
    return { m_roof, m_projection, m_shade };
}

void rsai::building_models::prismatic::transform_2_raster ( const Eigen::Matrix3d &transform, const Eigen::Vector2d &tile_shift )
{
    auto roof_on_raster = gdal::operator * ( m_roof, transform );
    auto proj_on_raster = gdal::operator * ( m_projection, transform );
    auto shade_on_raster = gdal::operator * ( m_shade, transform );

    m_roof = gdal::operator +( roof_on_raster, tile_shift );
    m_projection = gdal::operator +( proj_on_raster, tile_shift );
    m_shade = gdal::operator +( shade_on_raster, tile_shift );

    m_proj_pixel = transform.block < 2, 2 > ( 0, 0 ) * m_proj_step;
    m_shade_pixel = transform.block < 2, 2 > ( 0, 0 ) * m_shade_step;

    double l1_norm = std::max ( std::max ( std::abs ( m_proj_pixel.x() ), std::abs ( m_proj_pixel.y() ) )
                                , std::max ( std::abs ( m_shade_pixel.x() ), std::abs ( m_shade_pixel.y() ) ) );
    m_proj_pixel /= l1_norm;
    m_shade_pixel /= l1_norm;

    //std::cout << "m_proj_pixel " << m_proj_pixel << " m_shade_pixel " << m_shade_pixel << '\n';
}

void rsai::building_models::prismatic::transform_2_world ( const Eigen::Matrix3d &transform, const Eigen::Vector2d &tile_shift )
{
    auto roof_on_raster = gdal::operator +( m_roof, tile_shift );
    auto proj_on_raster = gdal::operator +( m_projection, tile_shift );
    auto shade_on_raster = gdal::operator +( m_shade, tile_shift );

    m_roof = gdal::operator * ( roof_on_raster, transform );
    m_projection = gdal::operator * ( proj_on_raster, transform );
    m_shade = gdal::operator * ( shade_on_raster, transform );
}

rsai::building_models::prismatic::estimates rsai::building_models::prismatic::estimate ( const cv::Mat &tile, const double segmentize_step, double &memory_factor
                                                                                         , const std::vector < cv::Mat > &segments, const double proj_sigma
                                                                                         , const double shade_sigma ) const
{
    return estimate ( get (), Eigen::Matrix3d::Identity(), Eigen::Vector2d::Zero(), m_proj_pixel, m_shade_pixel, tile
                      , segmentize_step, memory_factor, segments, proj_sigma, shade_sigma );
}

rsai::building_models::prismatic::estimates rsai::building_models::prismatic::estimate ( gdal::multipolygons structure, const Eigen::Matrix3d &world_2_raster
                                                                                         , const Eigen::Vector2d &shift
                                                                                         , const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step
                                                                                         , const cv::Mat &tile, const double segmentize_step, double &memory_factor
                                                                                         , const std::vector < cv::Mat > &segments, const double proj_sigma
                                                                                         , const double shade_sigma ) const
{
    auto roof_shape_pixel = structure [0] = gdal::operator +( gdal::operator *( structure [0], world_2_raster ), shift );
    auto proj_shape_pixel = structure [1] = gdal::operator +( gdal::operator *( structure [1], world_2_raster ), shift );
    auto shade_shape_pixel = structure [2] = gdal::operator +( gdal::operator *( structure [2], world_2_raster ), shift );

    Eigen::Vector2d proj_pixel = world_2_raster.block < 2, 2 > ( 0, 0 ) * proj_step;
    Eigen::Vector2d shade_pixel = world_2_raster.block < 2, 2 > ( 0, 0 ) * shade_step;

    roof_shape_pixel->segmentize ( segmentize_step );
    proj_shape_pixel->segmentize ( segmentize_step );
    shade_shape_pixel->segmentize ( segmentize_step );

    const auto proj_angle = eigen::to_polar ( proj_pixel ) [1] + M_PI;
    const auto shade_angle = eigen::to_polar ( shade_pixel ) [1] + M_PI;

    const int proj_mask_half = std::round ( proj_sigma * 3.0 );
    const int shade_mask_half = std::round ( shade_sigma * 3.0 );
    convolution_mask proj_mask ( gauss::first_directed_derivative ( proj_angle, proj_sigma, 0.5 ), proj_mask_half, proj_mask_half );
    convolution_mask shade_mask ( gauss::first_directed_derivative ( shade_angle, shade_sigma, 0.5 ), shade_mask_half, shade_mask_half );
    convolution_mask shade_mask_90_cw ( gauss::first_directed_derivative ( shade_angle + M_PI_2, shade_sigma, 0.5 ), shade_mask_half, shade_mask_half );
    convolution_mask shade_mask_90_ccw ( gauss::first_directed_derivative ( shade_angle - M_PI_2, shade_sigma, 0.5 ), shade_mask_half, shade_mask_half );

    std::pair < double, int > proj_respose ( 0.0, 0 );

    if ( segments.size() )
    {
        std::pair < double, int > curr_proj_respose ( 0.0, 1 );
        for ( const auto &segment : segments )
        {
            for ( auto polygon : proj_shape_pixel.get () )
            {
                for ( auto ring : polygon )
                {
                    for ( const auto &point : ring )
                    {
                        const int x = int ( point.getX () );
                        const int y = int ( point.getY () );

                        auto response = segment.at < uint8_t > ( y, x );
                        curr_proj_respose.first += response;
                        //++curr_proj_respose.second;
                    }
                }
            }
        }

        if ( curr_proj_respose.first > proj_respose.first )
            proj_respose = curr_proj_respose;
    }
    else
    {
        for ( auto polygon : proj_shape_pixel.get () )
        {
            for ( auto ring : polygon )
            {
                for ( const auto &point : ring )
                {
                    const int x = int ( point.getX () );
                    const int y = int ( point.getY () );

                    auto response = proj_mask.operator() < uint8_t > ( tile, x, y );
                    proj_respose.first += ( response > 0.0 ) ? response : 0.0;
                    ++proj_respose.second;
                }
            }
        }
    }

    std::pair < double, int > shade_respose ( 0.0, 0 );
    //std::pair < double, int > shade_factor ( 0.0, 0 );

    std::pair < double, int > brightness ( 0, 0 );

    for ( auto polygon : shade_shape_pixel.get () )
    {
        for ( auto ring : polygon )
        {
            for ( const auto &point : ring )
            {
                const int x = int ( point.getX () );
                const int y = int ( point.getY () );

                /*std::pair < int, int > brightness ( 0, 0 );
                for ( auto weight : shade_mask )
                {
                    if ( weight.value < 0.0 )
                    {
                        brightness.first += tile.at < uint8_t > ( int ( y + weight.at.y ), int ( x + weight.at.x ) );
                        brightness.second += 1;
                    }
                }

                const auto mean_brightness = ( brightness.first / brightness.second ) / 255.0;
                const double shade_weight = 1.0 / ( 1.0 + 1.0 / std::exp ( -20 * ( mean_brightness - 0.3 ) ) );

                shade_factor.first += shade_weight;
                ++shade_factor.second;*/

                const double shade_weight = 1.0;

                auto response_direct = shade_mask.operator() < uint8_t > ( tile, x, y );
                auto response_90cw = shade_mask_90_cw.operator() < uint8_t > ( tile, x, y );
                auto response_90ccw = shade_mask_90_ccw.operator() < uint8_t > ( tile, x, y );

                auto response = std::max ( std::max ( response_90cw, response_90ccw ), response_direct );

                shade_respose.first += ( response > 0.0 ) ? response * shade_weight : 0.0;
                ++shade_respose.second;
            }

            gdal::bbox bbox ( ring );

            int from_x = bbox.top_left().x();
            int from_y = bbox.top_left().y();
            int to_x = bbox.bottom_right().x();
            int to_y = bbox.bottom_right().y();

            from_x = ( from_x < 0 ) ? 0 : from_x;
            from_y = ( from_y < 0 ) ? 0 : from_y;
            to_x = ( to_x >= tile.cols ) ? tile.cols - 1 : to_x;
            to_y = ( to_y >= tile.rows ) ? tile.rows - 1 : to_y;

            cv::Mat render = cv::Mat::zeros ( tile.size (), CV_8U );
            __render_polygon_2_mat ( polygon, render, 0xFF );

            for ( int y = from_y; y <= to_y; ++y )
            {
                for ( int x = from_x; x <= to_x; ++x )
                {
                    /*OGRPoint pt ( x, y );
                    if ( polygon->Contains ( &pt ) )
                    {
                        brightness.first += tile.at < uint8_t > ( y, x );
                        brightness.second += 1;
                    }*/

                    if ( render.at < uint8_t > ( y, x ) )
                    {
                        brightness.first += tile.at < uint8_t > ( y, x );
                        brightness.second += 1;
                    }
                }
            }
        }
    }

    const auto mean_brightness = ( brightness.second != 0 ) ? ( brightness.first / brightness.second ) / 255.0 : 1.0;
    const double shade_weight = 1.0 / ( 1.0 + 1.0 / std::exp ( -20 * ( mean_brightness - 0.3 ) ) );

    shade_respose.first *= shade_weight;
    memory_factor = shade_weight;

    //memory_factor += shade_factor.first / shade_factor.second - 0.5;

    return { proj_respose.first / proj_respose.second, shade_respose.first / shade_respose.second, shade_weight };
}

multipolygon rsai::building_models::prismatic::projection () const
{
    return m_projection;
}

multipolygon rsai::building_models::prismatic::shade () const
{
    return m_shade;
}

multipolygon rsai::building_models::prismatic::roof () const
{
    return m_roof;
}

rsai::building_models::prismatic & rsai::building_models::prismatic::operator = ( const prismatic &src )
{
    abstract::operator = ( src );

    m_projection = ( src.m_projection ) ? instance ( src.m_projection->clone () ) : nullptr;
    m_shade = ( src.m_shade ) ? instance ( src.m_shade->clone () ) : nullptr;
    m_roof = ( src.m_roof ) ? instance ( src.m_roof->clone () ) : nullptr;

    m_proj_pixel = src.m_proj_pixel;
    m_shade_pixel = src.m_shade_pixel;

    return *this;
}

double rsai::building_models::prismatic::__line_polar_distance ( const Eigen::Vector2d &p1, const Eigen::Vector2d &p2 ) const
{
    Eigen::Matrix2d X;
    X << p1 [0], 1.0
       , p2 [0], 1.0;
    Eigen::Vector2d Y { p1 [1], p2 [1] };

    const Eigen::Vector2d K = X.colPivHouseholderQr().solve ( Y );

    const double A = K ( 0, 0 ),
                 B = -1.0,
                 C = K ( 1, 0 );
    const double p = C / sqrt ( A * A + B * B );
    return p;
}

bool rsai::building_models::prismatic::__render_polygon_2_mat ( OGRPolygon* polygon, cv::Mat& image, uint8_t color ) const
{
    if (polygon == nullptr || image.empty())
        return false;

    OGRLinearRing* exteriorRing = polygon->getExteriorRing();
    if (exteriorRing == nullptr)
        return false;

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Point> cvPoints;
    for (int i = 0; i < exteriorRing->getNumPoints(); ++i)
    {
        OGRPoint ogrPoint;
        exteriorRing->getPoint(i, &ogrPoint);
        cvPoints.emplace_back(static_cast<int>(ogrPoint.getX()), static_cast<int>(ogrPoint.getY()));
    }
    contours.push_back(cvPoints);

    if (cvPoints.size() < 3)
        return false;

    cv::fillPoly(image, contours, cv::Scalar(color));

    return true;
}
