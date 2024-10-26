#pragma once

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <gdal_utils/shared_geometry.h>

namespace geometry
{
    class point_fill
    {
    public:
        struct markup
        {
            gdal::multipoint positive;
            gdal::multipoint negative;
        }; // struct markup

        point_fill ( const OGRPolygon * polygon );
        point_fill ( gdal::polygon polygon );

        markup operator () () const;
    private:
        gdal::polygon m_polygon;
    };
}
