#include "eigen_utils/geometry.h"

#include <limits>

#include "eigen_utils/gdal_bridges.h"

using namespace gdal;

gdal::bbox::bbox ()
    : m_top_left ( 0, 0 ), m_bottom_right ( 0, 0 )
{

}

gdal::bbox::bbox ( const Eigen::Vector2d &tl, const Eigen::Vector2d &br )
    : m_top_left ( tl ), m_bottom_right ( br )
{
}

gdal::bbox::bbox ( const polygon &polygon )
    : bbox ( polygon.get () )
{
}

gdal::bbox::bbox ( const OGRPolygon *polygon )
{
    double min_x = std::numeric_limits < double >::max (),
           min_y = std::numeric_limits < double >::max (),
           max_x = std::numeric_limits < double >::lowest (),
           max_y = std::numeric_limits < double >::lowest ();
    for ( const auto ring : *polygon )
    {
        for ( const auto pt : *ring )
        {
            min_x = std::min ( min_x, pt.getX () );
            min_y = std::min ( min_y, pt.getY () );
            max_x = std::max ( max_x, pt.getX () );
            max_y = std::max ( max_y, pt.getY () );
        }
    }
    m_top_left << min_x, min_y;
    m_bottom_right << max_x, max_y;
}

gdal::bbox::bbox ( const geometry &geometry )
    : bbox ( geometry.get () )
{

}

gdal::bbox::bbox ( const OGRGeometry *geometry )
{
    OGREnvelope envelope;
    geometry->getEnvelope(&envelope);
    m_top_left      << envelope.MinX, envelope.MinY;
    m_bottom_right  << envelope.MaxX, envelope.MaxY ;
}

gdal::bbox::bbox ( const std::list < bbox > &bboxes )
{
    double min_x = std::numeric_limits < double >::max (),
           min_y = std::numeric_limits < double >::max (),
           max_x = std::numeric_limits < double >::lowest (),
           max_y = std::numeric_limits < double >::lowest ();
    for ( const auto &bbox : bboxes )
    {
        const auto &tl = bbox.top_left ();
        const auto &br = bbox.bottom_right ();

        min_x = std::min ( min_x, tl.x () );
        min_y = std::min ( min_y, tl.y () );
        max_x = std::max ( max_x, br.x () );
        max_y = std::max ( max_y, br.y () );
    }

    m_top_left << min_x, min_y;
    m_bottom_right << max_x, max_y;
}

polygon gdal::bbox::transform  ( const Eigen::Matrix3d &transform ) const
{
    const auto bbox_polygon         = to_polygon ();
    const auto transformed_polygon  = gdal::operator * ( bbox_polygon, transform );
    return transformed_polygon;
}

polygon gdal::bbox::transform  ( const Eigen::Matrix2d &transform ) const
{
    const auto bbox_polygon         = to_polygon ();
    const auto transformed_polygon  = gdal::operator * ( bbox_polygon, transform );
    return transformed_polygon;
}

void gdal::bbox::bufferize  ( const Eigen::Vector2d &buffer_sizes )
{
    m_top_left += -buffer_sizes;
    m_bottom_right += buffer_sizes;
}

polygon gdal::bbox::to_polygon () const
{
    auto result = instance < polygon > ();

    auto new_ring = new OGRLinearRing;
    new_ring->addPoint ( m_top_left.x (), m_top_left.y () );
    new_ring->addPoint ( m_bottom_right.x (), m_top_left.y () );
    new_ring->addPoint ( m_bottom_right.x (), m_bottom_right.y () );
    new_ring->addPoint ( m_top_left.x (), m_bottom_right.y () );

    result->addRingDirectly ( new_ring );
    result->closeRings ();
    return result;
}

bool gdal::bbox::verify ( gdal::shared_dataset raster )
{
    if ( !raster )
        return false;

    const int bands = raster->GetRasterCount();

    if ( bands == 0 )
        return false;

    auto first_band = raster->GetRasterBand ( 1 );

    m_top_left [0] = std::max ( m_top_left.x (), 0.0 );
    m_top_left [1] = std::max ( m_top_left.y (), 0.0 );
    m_bottom_right [0] = std::min ( m_bottom_right.x (), static_cast < double > ( first_band->GetXSize() - 1 ) );
    m_bottom_right [1] = std::min ( m_bottom_right.y (), static_cast < double > ( first_band->GetYSize() - 1 ) );

    if ( m_top_left.x () > m_bottom_right.x () || m_top_left.y () > m_bottom_right.y () )
    {
        m_top_left << 0.0, 0.0;
        m_bottom_right << 0.0, 0.0;
        return false;
    }
    else
        return true;
}

bool gdal::bbox::empty () const
{
    return  m_top_left == Eigen::Vector2d { 0, 0 } && m_bottom_right == Eigen::Vector2d { 0, 0 };
}

void gdal::bbox::set ( const Eigen::Vector2d &tl, const Eigen::Vector2d &br )
{
    m_top_left = tl;
    m_bottom_right = br;
}

const Eigen::Vector2d & gdal::bbox::top_left () const
{
    return m_top_left;
}

const Eigen::Vector2d & gdal::bbox::bottom_right () const
{
    return m_bottom_right;
}

const Eigen::Vector2d gdal::bbox::center () const
{
    return ( m_top_left + m_bottom_right ) / 2;
}

const Eigen::Vector2d gdal::bbox::size () const
{
    return m_bottom_right - m_top_left;
}

bbox & gdal::bbox::operator += ( const Eigen::Vector2d & rh )
{
    m_top_left += rh;
    m_bottom_right += rh;
    return *this;
}

OGRPolygon * gdal::operator += ( OGRPolygon *polygon, const Eigen::Vector2d &shift )
{
    const auto shift_x = shift [0],
               shift_y = shift [1];

    for ( const auto ring : *polygon )
    {
        for ( auto &pt : *ring )
        {
            pt.setX ( pt.getX () + shift_x );
            pt.setY ( pt.getY () + shift_y );
        }
    }
    return polygon;
}

polygon & gdal::operator += ( polygon &polygon, const Eigen::Vector2d &shift )
{
    polygon.get () += shift;
    return polygon;
}

polygons& gdal::operator += ( polygons &polys, const Eigen::Vector2d &shift )
{
    for ( auto &poly : polys )
        poly += shift;
    return polys;
}

OGRPolygon * gdal::operator *=  ( OGRPolygon *polygon, const Eigen::Matrix3d &transform )
{
    for ( auto ring : *polygon )
    {
        for ( auto &pt : *ring )
        {
            Eigen::Vector3d vec ( pt.getX (), pt.getY (), 1 );
            vec = transform * vec;
            pt.setX ( vec [0] );
            pt.setY ( vec [1] );
        }
    }
    return polygon;
}

OGRMultiPolygon * gdal::operator *=  ( OGRMultiPolygon *polygon, const Eigen::Matrix3d &transform )
{
    for ( auto item : polygon )
    {
        item *= transform;
    }
    return polygon;
}

polygon & gdal::operator *=  ( polygon &polygon, const Eigen::Matrix3d &transform )
{
    polygon.get () *= transform;
    return polygon;
}

polygons& gdal::operator *=  ( polygons &polys, const Eigen::Matrix3d &transform )
{
    for ( auto &poly : polys )
        poly *= transform;
    return polys;
}

geometry gdal::operator * ( OGRGeometry *geom, const Eigen::Matrix3d &transform )
{
    if ( gdal::is_polygon ( geom ) )
        return geometry ( geom->clone ()->toPolygon() * transform );

    if ( gdal::is_multipolygon ( geom ) )
        return geometry ( geom->clone ()->toMultiPolygon() * transform );

    return {};
}

point gdal::operator + ( const point &point, const Eigen::Vector2d &shift )
{
    return gdal::point ( new OGRPoint ( point->getX () + shift [0], point->getY () + shift [1] ) );
}

point gdal::operator + ( const OGRPoint *point, const Eigen::Vector2d &shift )
{
    return gdal::point ( new OGRPoint ( point->getX () + shift [0], point->getY () + shift [1] ) );
}

multipoint gdal::operator + ( const OGRMultiPoint *point, const Eigen::Vector2d &shift )
{
    auto result = instance < multipoint > ();
    for ( auto item : point )
    {
        auto new_item = gdal::operator +( item, shift );
        result->addGeometryDirectly( new_item->clone() );
    }
    return result;
}

multipoint gdal::operator + ( const multipoint &point, const Eigen::Vector2d &shift )
{
    return gdal::operator + ( point.get(), shift );
}

polygon gdal::operator + ( const OGRPolygon *polygon, const Eigen::Vector2d &shift )
{
    const auto shift_x = shift [0],
               shift_y = shift [1];

    gdal::polygon result ( new polygon::element_type );
    for ( const auto ring : *polygon )
    {
        auto new_ring = new OGRLinearRing;
        for ( const auto pt : *ring )
        {
            new_ring->addPoint ( pt.getX () + shift_x, pt.getY () + shift_y );
        }
        result->addRingDirectly ( new_ring );
    }
    return result;
}

polygon gdal::operator + ( const polygon &polygon, const Eigen::Vector2d &shift )
{
    return gdal::operator + ( polygon.get (), shift );
}

multipolygon gdal::operator +  ( const OGRMultiPolygon *polygon, const Eigen::Vector2d &shift )
{
    auto result = instance < multipolygon > ();
    for ( auto item : polygon )
    {
        auto new_item = gdal::operator +( item, shift );
        result->addGeometryDirectly( new_item->clone() );
    }
    return result;
}

multipolygon gdal::operator +  ( const multipolygon &polygon, const Eigen::Vector2d &shift )
{
    return gdal::operator + ( polygon.get(), shift );
}

geometry gdal::operator + ( OGRGeometry *geom, const Eigen::Vector2d &shift )
{
    if ( gdal::is_polygon ( geom ) )
        return geometry ( geom->clone ()->toPolygon() + shift );

    if ( gdal::is_multipolygon ( geom ) )
        return geometry ( geom->clone ()->toMultiPolygon() + shift );

    return {};
}

geometry gdal::operator + ( geometry geom, const Eigen::Vector2d &shift )
{
    return geom.get () + shift;
}

point gdal::operator * ( const OGRPoint *point, const Eigen::Matrix3d &transform )
{
    Eigen::Vector3d vec ( point->getX (), point->getY (), 1 );
    vec = transform * vec;
    return gdal::point ( new OGRPoint ( vec [0], vec [1] ) );
}

point gdal::operator * ( const point &point, const Eigen::Matrix3d &transform )
{
    return gdal::operator * ( point.get (), transform );
}

multipoint gdal::operator * ( const OGRMultiPoint *point, const Eigen::Matrix3d &transform )
{
    auto result = instance < multipoint > ();
    for ( auto item : point )
    {
        auto new_item = gdal::operator *( item, transform );
        result->addGeometryDirectly( new_item->clone() );
    }
    return result;
}

multipoint gdal::operator * ( const multipoint &point, const Eigen::Matrix3d &transform )
{
    return gdal::operator * ( point.get(), transform );
}

polygon gdal::operator * ( const OGRPolygon *polygon, const Eigen::Matrix3d &transform )
{
    gdal::polygon result ( new polygon::element_type );
    for ( const auto ring : *polygon )
    {
        auto new_ring = new OGRLinearRing;
        for ( const auto pt : *ring )
        {
            Eigen::Vector3d vec ( pt.getX (), pt.getY (), 1 );
            vec = transform * vec;
            new_ring->addPoint ( vec [0], vec [1] );
        }
        result->addRingDirectly ( new_ring );
    }
    return result;
}

polygon gdal::operator *  ( const polygon &polygon, const Eigen::Matrix3d &transform )
{
    return gdal::operator * ( polygon.get (), transform );
}

multipolygon gdal::operator *  ( const OGRMultiPolygon *polygon, const Eigen::Matrix3d &transform )
{
    auto result = instance < multipolygon > ();
    for ( auto item : polygon )
    {
        auto new_item = gdal::operator *( item, transform );
        result->addGeometryDirectly( new_item->clone() );
    }
    return result;
}

multipolygon gdal::operator *  ( const multipolygon &polygon, const Eigen::Matrix3d &transform )
{
    return gdal::operator * ( polygon.get(), transform );
}

polygon gdal::operator *  ( const OGRPolygon *polygon, const Eigen::Matrix2d &transform )
{
    gdal::polygon result ( new polygon::element_type );
    for ( const auto ring : *polygon )
    {
        auto new_ring = new OGRLinearRing;
        for ( const auto pt : *ring )
        {
            Eigen::Vector2d vec ( pt.getX (), pt.getY () );
            vec = transform * vec;
            new_ring->addPoint ( vec [0], vec [1] );
        }
        result->addRingDirectly ( new_ring );
    }
    return result;
}

polygon gdal::operator *  ( const polygon &polygon, const Eigen::Matrix2d &transform )
{
    return gdal::operator * ( polygon.get (), transform );
}

multipolygon gdal::operator *  ( const OGRMultiPolygon *polygon, const Eigen::Matrix2d &transform )
{
    auto result = instance < multipolygon > ();
    for ( auto item : polygon )
    {
        auto new_item = gdal::operator *( item, transform );
        result->addGeometryDirectly( new_item->clone() );
    }
    return result;
}

multipolygon gdal::operator *  ( const multipolygon &polygon, const Eigen::Matrix2d &transform )
{
    return gdal::operator * ( polygon.get(), transform );
}

polygon gdal::project ( const gdal::polygon &base, const Eigen::Vector2d &step, const int vector_len )
{
    const Eigen::Vector2d proj_vec = step * vector_len;
    point pt ( new point::element_type );

    auto base_ring = base->getExteriorRing ();

    base_ring->getPoint( 0, pt.get () );
    Eigen::Vector2d p1 = eigen::cast < double > ( pt );
    Eigen::Vector2d p4 = p1 + proj_vec;

    geometry result ( new OGRPolygon );
    for ( int i = 1; i < base_ring->getNumPoints (); ++i )
    {
        base_ring->getPoint( i, pt.get () );
        auto p2 = eigen::cast < double > ( pt );
        auto p3 = p2 + proj_vec;

        auto segment = new OGRLinearRing;
        segment->addPoint ( p1 [0], p1 [1] );
        segment->addPoint ( p2 [0], p2 [1] );
        segment->addPoint ( p3 [0], p3 [1] );
        segment->addPoint ( p4 [0], p4 [1] );
        segment->addPoint ( p1 [0], p1 [1] );

        p1 = p2; p4 = p3;

        auto segment_poly = instance < polygon > ();
        segment_poly->addRingDirectly ( segment );

        auto current_union = result->Union ( segment_poly.get () );
        result = instance ( current_union );
    }

    auto out = instance < polygon > ();

    if ( wkbFlatten ( result->getGeometryType()) == wkbPolygon )
    {
        out->addRing ( result->toPolygon()->getExteriorRing () );
    }
    else
    if ( wkbFlatten ( result->getGeometryType()) == wkbMultiPolygon )
    {
        auto polygon = result->toMultiPolygon ()->getGeometryRef( 0 );
        out->addRing ( polygon->getExteriorRing() );
    }
    return out;
}

polygon gdal::bbox_and_projections_2_polygon ( const OGRPolygon *polygon, const std::list < Eigen::Vector2d > &shifts, const Eigen::Vector2d &raster_sizes
                                                    , const Eigen::Matrix3d &raster_2_world, const double buffer_size, const bool save_projections )
{
    gdal::polygon result ( new polygon::element_type );
    std::list < bbox > bboxes;
    for ( const auto &shift : shifts )
    {
        auto shifted = gdal::operator + ( polygon, shift );
        bboxes.push_back ( bbox ( shifted ) );

        if ( save_projections )
            result << shifted;
    }

    const Eigen::Matrix3d world_2_raster = raster_2_world.inverse ();
    auto covering = bbox ( bboxes );
    auto covering_raster = bbox ( covering.transform( world_2_raster ) );
    covering_raster.bufferize( { buffer_size, buffer_size } );

    const auto &tl = covering_raster.top_left();
    const auto &br = covering_raster.bottom_right();

    if ( tl.x() < 0.0 || tl.y () < 0.0 )
        return {};

    if ( br.x () > raster_sizes [0] || br.y () > raster_sizes [1] )
        return {};

    result << covering_raster.transform( raster_2_world );

    return result;
}
