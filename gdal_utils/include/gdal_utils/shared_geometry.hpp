#pragma once

#include "gdal_utils/shared_geometry.h"
#include <fstream>
#include <iostream>

namespace gdal
{
    static bool is_geometry_type ( const OGRGeometry *geometry, OGRwkbGeometryType type )
    {
        if (geometry != nullptr)
        {
            OGRwkbGeometryType geomType = wkbFlatten(geometry->getGeometryType());
            return geomType == type ;
        }
        return false;
    }

    static bool is_geometry_type ( const geometry &geometry, OGRwkbGeometryType type )
    {
        return is_geometry_type ( geometry.get(), type );
    }

    static bool is_polygon ( const geometry &geometry )
    {
        return is_geometry_type ( geometry, wkbPolygon );
    }

    static bool is_polygon ( const OGRGeometry *geometry )
    {
        return is_geometry_type ( geometry, wkbPolygon );
    }

    static bool is_multipolygon ( const geometry &geometry )
    {
        return is_geometry_type ( geometry, wkbMultiPolygon );
    }

    static bool is_multipolygon ( const OGRGeometry *geometry )
    {
        return is_geometry_type ( geometry, wkbMultiPolygon );
    }

    static bool is_collection ( const geometry &geometry )
    {
        return is_geometry_type ( geometry, wkbGeometryCollection );
    }

    static bool is_collection ( const OGRGeometry *geometry )
    {
        return is_geometry_type ( geometry, wkbGeometryCollection );
    }

    static polygon to_polygon ( const geometry &geometry )
    {
        auto a_polygon = geometry->toPolygon();
        return polygon ( ( a_polygon ) ? a_polygon->clone() : nullptr );
    }

    static polygon to_polygon ( const OGRGeometry *geometry )
    {
        auto a_polygon = geometry->toPolygon();
        return polygon ( ( a_polygon ) ? a_polygon->clone() : nullptr );
    }

    static polygons to_polygon ( const geometries &geometries )
    {
        polygons result;
        result.reserve ( geometries.size() * 2 );
        for ( auto geometry : geometries )
        {

            if ( is_polygon ( geometry ) )
            {
                auto polygon = to_polygon ( geometry );

                if ( polygon )
                    result.push_back ( polygon );
            }

            if ( is_multipolygon ( geometry ) )
            {
                auto multipolygon = to_multipolygon ( geometry );

                const int polygons_count = multipolygon->getNumGeometries();
                for ( int i = 0; i < polygons_count; ++i )
                {
                    auto polygon = gdal::polygon ( multipolygon->getGeometryRef ( i )->clone() );
                    if ( polygon )
                        result.push_back ( polygon );
                }
            }

            if ( is_collection ( geometry ) )
            {
                auto collection = to_collection ( geometry );

                gdal::geometries items;
                for ( auto &item : collection.get() )
                    items.emplace_back ( item->clone() );

                auto polygons = to_polygon ( items );
                result.insert(result.end(), std::make_move_iterator(polygons.begin()), std::make_move_iterator(polygons.end()));
            }
        }

        return result;
    }

    static multipolygon to_multipolygon ( const geometry &geometry )
    {
        auto polygon = geometry->toMultiPolygon();
        return multipolygon ( ( polygon ) ? polygon->clone() : nullptr );
    }

    static multipolygon to_multipolygon ( const OGRGeometry *geometry )
    {
        auto polygon = geometry->toMultiPolygon();
        return multipolygon ( ( polygon ) ? polygon->clone() : nullptr );
    }

    static collection to_collection ( const geometry &geometry )
    {
        auto a_collection = geometry->toGeometryCollection();
        return collection ( ( a_collection ) ? a_collection->clone() : nullptr );
    }

    template < class PtrType >
    PtrType instance ()
    {
        return PtrType ( new typename PtrType::element_type );
    }

    template < class Type >
    std::shared_ptr < Type > instance ( Type * obj )
    {
        return std::shared_ptr < Type > ( obj );
    }

    static polygon operator << ( polygon to, const OGRPolygon *from )
    {
        auto *polygon = const_cast < OGRPolygon * > ( from );
        for ( const auto ring : *polygon )
        {
            to->addRing ( ring );
        }

        return to;
    }

    static polygon operator << ( polygon to, const polygon &from )
    {
        return ( to << from.get () );
    }

    static std::string to_wkt ( const OGRGeometry * geom )
    {
        char* wkt = nullptr;

        geom->exportToWkt(&wkt);
        std::string wkt_str = wkt;
        CPLFree(wkt);

        return std::move ( wkt_str );
    }

    static geometries from_wkt_file ( const std::string file_name, gdal::polygon spatial_filter )
    {
        CPLSetErrorHandler(CPLQuietErrorHandler);

        geometries geometries;
        std::ifstream file_stream(file_name);
        std::string wkt_string;

        if (!file_stream.is_open())
        {
            std::cerr << "Unable to open WKT file: " << file_name << std::endl;
            return geometries;
        }

        while ( std::getline ( file_stream, wkt_string ) )
        {
            if ( wkt_string.empty () )
                continue;

            OGRGeometry* a_geometry = nullptr;
            const char * wkt = const_cast < const char* > ( wkt_string.c_str () );

            OGRErr err = OGRGeometryFactory::createFromWkt ( &wkt, nullptr, &a_geometry );
            if (err != OGRERR_NONE)
            {
                std::cerr << "Error parsing WKT: " << wkt_string << std::endl;
                continue;
            }

            auto is_invalid = !a_geometry->IsValid();

            if ( !a_geometry->IsValid() )
            {
                a_geometry = gdal::geometry ( a_geometry )->MakeValid();
//                std::cout << file_name << " Geometry is invalid\n";
                //std::cout << "Invalid WKT " << wkt_string << '\n';
//                std::cout << "Valid WKT " << to_wkt ( geometry ) << '\n';
            }

            if ( spatial_filter )
            {
                a_geometry = gdal::geometry ( a_geometry )->Intersection ( spatial_filter.get () );

//                if ( is_invalid )
//                    std::cout << "Intersected WKT " << to_wkt ( geometry ) << '\n';
            }
            //else
                //std::cout << "Spatial filter is invalid\n";

            if ( a_geometry == nullptr )
            {
                std::cout << "Geometry is null\n";
                continue;
            }

            if ( !a_geometry->IsEmpty() )
                geometries.push_back ( gdal::geometry ( a_geometry ) );
        }

        file_stream.close();

        CPLSetErrorHandler(CPLDefaultErrorHandler);

        return geometries;
    }

    template < class Geometry_Type >
    bool to_wkt_file ( const std::string file_name, std::vector < Geometry_Type > geometry_list )
    {
        std::ofstream outFile(file_name);
        if ( !outFile.is_open() )
            return false;

        char* wkt = nullptr;
        for (const auto& geom : geometry_list)
        {
            if (geom != nullptr)
            {
                OGRErr err = geom->exportToWkt(&wkt);
                if (err != OGRERR_NONE)
                {
                    CPLFree(wkt);
                    return false;
                }
                outFile << wkt << std::endl;
                CPLFree(wkt);
            }
        }

        outFile.close();
        return true;
    }

} // namespace gdal
