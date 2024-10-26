#include "geometry_utils/points_fill.h"

#include <random>
#include "eigen_utils/geometry.h"

typedef boost::geometry::model::d2::point_xy<double> point_t;
typedef boost::geometry::model::polygon<point_t> polygon_t;

typedef std::pair<point_t, unsigned> value_t;
typedef boost::geometry::index::rtree<value_t, boost::geometry::index::quadratic<16>> rtree_t;

bool check_min_distance(const boost::geometry::index::rtree<value_t, boost::geometry::index::quadratic<16>>& rtree, const point_t& point, double min_distance)
{
    std::vector<value_t> result;
    rtree.query(boost::geometry::index::nearest(point, 1), std::back_inserter(result));

    if (result.empty())
        return true;

    return boost::geometry::distance(result[0].first, point) >= min_distance;
}

polygon_t ogr_2_boost_polygon (gdal::polygon polygon)
{
    polygon_t boost_polygon;

    OGRLinearRing* exteriorRing = polygon->getExteriorRing();

    for(int i = 0; i < exteriorRing->getNumPoints(); i++)
        boost::geometry::append(boost_polygon, point_t (exteriorRing->getX(i), exteriorRing->getY(i)));

    for(int i = 0; i < polygon->getNumInteriorRings(); i++)
    {
        OGRLinearRing* interiorRing = polygon->getInteriorRing(i);
        boost::geometry::model::ring<point_t> boostInteriorRing;

        for(int j = 0; j < interiorRing->getNumPoints(); j++)
            boost::geometry::append(boostInteriorRing, point_t(interiorRing->getX(j), interiorRing->getY(j)));

        boost_polygon.inners().push_back(boostInteriorRing);
    }

    return boost_polygon;
}

geometry::point_fill::point_fill ( const OGRPolygon * polygon )
    : m_polygon ( polygon->clone () )
{

}

geometry::point_fill::point_fill ( gdal::polygon polygon )
    : m_polygon ( polygon )
{
}

geometry::point_fill::markup geometry::point_fill::operator () () const
{
    // Define your parameters for point sampling and distance calculation
    double step_size = 1.0;
    double min_negative_distance = 30.0;
    double min_internal_distance = 15.0;
    double min_external_distance = 7.0;
    double bbox_enlarge_value = 50;

    gdal::polygon ogr_poly ( m_polygon->clone () );
    gdal::bbox poly_bbox ( ogr_poly );
    poly_bbox.bufferize ( { bbox_enlarge_value, bbox_enlarge_value } );
    ogr_poly->segmentize ( step_size );

    auto poly = ogr_2_boost_polygon ( ogr_poly );

    point_t center;
    boost::geometry::centroid ( poly, center );

    rtree_t internal;
    rtree_t external;
    rtree_t negative;

    // prefill rtree with polygon
    for ( auto const& point: boost::geometry::exterior_ring (poly) )
        external.insert ( std::make_pair ( point, 0 ) );

    markup result;
    result.positive = gdal::instance < gdal::multipoint > ();
    result.negative = gdal::instance < gdal::multipoint > ();

    std::random_device rd;
    std::mt19937 eng(rd());

    const int bbox_square = ( poly_bbox.bottom_right().x () - poly_bbox.top_left().x () )
            * ( poly_bbox.bottom_right().y () - poly_bbox.top_left().y () );
    const int samples_count = bbox_square / 10;
    std::uniform_int_distribution<> dx (poly_bbox.top_left().x (), poly_bbox.bottom_right().x ());
    std::uniform_int_distribution<> dy (poly_bbox.top_left().y (), poly_bbox.bottom_right().y ());

    //for (double y = poly_bbox.top_left().y (); y <= poly_bbox.bottom_right().y (); y += step_size)
    for ( int i = 0; i < samples_count; ++i )
    {
        //for (double x = poly_bbox.top_left().x (); x <= poly_bbox.bottom_right().x (); x += step_size)
        {
            const double x = dx ( eng );
            const double y = dy ( eng );

            point_t point{x, y};
            if (boost::geometry::within(point, poly) && check_min_distance(internal, point, min_internal_distance)
                    && check_min_distance(external, point, min_external_distance) )
            {
                auto ogr_point = new OGRPoint ( x, y );

                result.positive->addGeometryDirectly( ogr_point );
                internal.insert(std::make_pair(point, 0));
            }

            if (!boost::geometry::within(point, poly) && check_min_distance(negative, point, min_negative_distance)
                    && check_min_distance(external, point, min_external_distance) )
            {
                auto ogr_point = new OGRPoint ( x, y );

                result.negative->addGeometryDirectly( ogr_point );
                negative.insert(std::make_pair(point, 0));
            }
        }
    }

    if ( result.positive->getNumGeometries() == 0 )
    {
        auto ogr_center = new OGRPoint ( center.x (), center.y () );
        result.positive->addGeometryDirectly( ogr_center );
    }

    return std::move ( result );
}

