#include "rsai/building_models/abstract.h"

using namespace gdal;
using namespace rsai::building_models;

rsai::building_models::abstract::abstract ( const OGRLinearRing *object, const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step )
    : m_object ( instance < polygon > () ), m_proj_step ( proj_step ), m_shade_step ( shade_step )
{
    m_object->addRingDirectly( object->clone () );
}

rsai::building_models::abstract::abstract ( gdal::polygon object, const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step )
    : m_object ( object ), m_proj_step ( proj_step ), m_shade_step ( shade_step )
{

}

rsai::building_models::abstract::abstract ( const abstract &src )
    : m_proj_step ( src.m_proj_step ), m_shade_step ( src.m_shade_step )
{
    if ( src.m_object )
        m_object = instance ( src.m_object->clone() );
}

rsai::building_models::abstract & rsai::building_models::abstract::operator = ( const abstract &src )
{
    m_object = src.m_object ? instance ( src.m_object->clone() ) : nullptr;

    m_proj_step = src.m_proj_step;
    m_shade_step = src.m_shade_step;

    return *this;
}

Eigen::Vector2d rsai::building_models::abstract::projection_step () const
{
    return m_proj_step;
}

Eigen::Vector2d rsai::building_models::abstract::shade_step () const
{
    return m_shade_step;
}

gdal::multipolygons rsai::building_models::precalculated::get ( const int length ) const
{
    const int index = length - m_base_index;
    if ( index < 0 || index > m_items.size() )
        return {};
    else
        return m_items [index];
}

rsai::building_models::abstract::estimates rsai::building_models::precalculated::estimate ( const int length, const Eigen::Vector2d &shift
                                               , const cv::Mat &tile, const double segmentize_step, double &memory_factor
                                               , const std::vector < cv::Mat > &segments ) const
{
    auto structure = get ( length );
    return m_model->estimate ( structure, Eigen::Matrix3d::Identity(), shift, m_model->projection_step(), m_model->shade_step()
                               , tile, segmentize_step, memory_factor, segments );
}

double rsai::building_models::multiview::estimate ( const int length, const std::vector < Eigen::Vector2d > &tile_shifts
                               , const std::vector < cv::Mat > &tiles, const double segmentize_step, double &memory_factor
                               , const std::vector < cv::Mat > &segments ) const
{
    double estimate = 0.0;
    std::pair < double, int > shade_weights ( 0.0, 0 );
    for ( int i = 0; i < m_models.size(); ++i )
    {
        auto model = m_models [i];
        auto tile = tiles [i];
        if ( model && !tile.empty() )
        {
            tile = tile.clone();
            cv::cvtColor ( tile, tile, cv::COLOR_BGR2GRAY );

            double shade_weight = 0.0;
            auto current_estimates = model->estimate ( length, tile_shifts [i], tile, segmentize_step, shade_weight, segments );
            estimate += current_estimates [0] * current_estimates [1] * current_estimates [2];
            shade_weights.first += shade_weight;
            ++shade_weights.second;
        }
    }

    memory_factor = shade_weights.first / shade_weights.second;

    return estimate;
}

precalculated_models rsai::building_models::multiview::models () const
{
    return m_models;
}

std::vector < gdal::multipolygons > rsai::building_models::multiview::get ( const int length ) const
{
    std::vector < gdal::multipolygons > geometries;
    for ( auto model : m_models )
    {
        if ( model )
            geometries.push_back ( model->get ( length ) );
        else
            geometries.push_back ( { {}, {}, {} } );
    }

    return std::move ( geometries );
}

const int rsai::building_models::multiview::range_from () const
{
    return m_from;
}

const int rsai::building_models::multiview::range_to () const
{
    return m_to;
}
