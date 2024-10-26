#pragma once

#include <memory>
#include <vector>
#include <string>
#include <gdal_priv.h>

namespace gdal
{
    using geometry              = std::shared_ptr < OGRGeometry >;
    using point                 = std::shared_ptr < OGRPoint >;
    using curve                 = std::shared_ptr < OGRCurve >;
    using line                  = std::shared_ptr < OGRLineString >;
    using polygon               = std::shared_ptr < OGRPolygon >;
    using ring                  = std::shared_ptr < OGRLinearRing >;
    using multipoint            = std::shared_ptr < OGRMultiPoint >;
    using multiline             = std::shared_ptr < OGRMultiLineString >;
    using multipolygon          = std::shared_ptr < OGRMultiPolygon >;
    using collection            = std::shared_ptr < OGRGeometryCollection >;

    using geometries            = std::vector < geometry >;
    using polygons              = std::vector < polygon >;
    using multipolygons         = std::vector < multipolygon >;

    static bool                 is_geometry_type ( const OGRGeometry *geometry, OGRwkbGeometryType type );
    static bool                 is_geometry_type ( const geometry &geometry, OGRwkbGeometryType type );

    static bool                 is_polygon ( const OGRGeometry *geometry );
    static bool                 is_polygon ( const geometry &geometry );
    static bool                 is_multipolygon ( const OGRGeometry *geometry );
    static bool                 is_multipolygon ( const geometry &geometry );
    static bool                 is_collection ( const OGRGeometry *geometry );
    static bool                 is_collection ( const geometry &geometry );

    static polygon              to_polygon ( const OGRGeometry *geometry );
    static polygon              to_polygon ( const geometry &geometry );
    static polygons             to_polygon ( const geometries &geometries );

    static multipolygon         to_multipolygon ( const OGRGeometry *geometry );
    static multipolygon         to_multipolygon ( const geometry &geometry );
    static collection           to_collection ( const geometry &geometry );

    template < class PtrType >
    PtrType instance ();

    template < class Type >
    std::shared_ptr < Type > instance ( Type * obj );

    static polygon operator << ( polygon to, const OGRPolygon *from );
    static polygon operator << ( polygon to, const polygon &from );

    static std::string to_wkt ( const OGRGeometry * geom );

    static geometries from_wkt_file ( const std::string file_name, gdal::polygon spatial_filter = {} );

    template < class Geometry_Type >
    bool to_wkt_file ( const std::string file_name, std::vector < Geometry_Type > geometry_list );

}; // namespace gdal

#include "gdal_utils/shared_geometry.hpp"
