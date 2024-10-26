#pragma once

#include "gdal_utils/shared_geometry.h"
#include <cpl_conv.h>

namespace gdal
{
    template < class GeometryType1, class GeometryType2 >
    double iou ( const std::shared_ptr < GeometryType1 > &geom1, const std::shared_ptr < GeometryType2 > &geom2 );

    template < class GeometryType1, class GeometryType2 >
    double iou ( GeometryType1 * geom1, GeometryType2 * geom2 );

    static double get_area ( const gdal::geometry &geom );

    static double get_area ( const gdal::geometry &geom )
    {
        if (geom != nullptr)
        {
            OGRwkbGeometryType gType = wkbFlatten(geom->getGeometryType());

            if (gType == wkbPolygon)
                return geom->toPolygon ()->get_Area();
            else if (gType == wkbMultiPolygon)
                return geom->toMultiPolygon ()->get_Area();
            else if (gType == wkbGeometryCollection)
                return geom->toGeometryCollection()->get_Area();
        }
        return 0.0;
    }

    template < class GeometryType1, class GeometryType2 >
    double iou ( GeometryType1 * geom1, GeometryType2 * geom2 )
    {
        CPLSetErrorHandler(CPLQuietErrorHandler);

        if (geom1 == nullptr || geom2 == nullptr)
        {
            return 0.0;
        }

        if ( !geom1->IsValid() )
            geom1 = geom1->MakeValid();

        if ( !geom2->IsValid() )
            geom2 = geom2->MakeValid();

        gdal::geometry intersection_geom ( geom1->Intersection ( geom2 ) );
        gdal::geometry union_geom ( geom1->Union ( geom2 ) );

        if (intersection_geom == nullptr || union_geom == nullptr)
        {
            return 0.0;
        }

        if ( !intersection_geom->IsValid() )
            intersection_geom = gdal::geometry ( intersection_geom->MakeValid() );

        if ( !union_geom->IsValid() )
            union_geom = gdal::geometry ( union_geom->MakeValid() );

        const double intersectionArea = get_area ( intersection_geom );
        const double unionArea = get_area ( union_geom );

        if (unionArea == 0.0)
        {
            return 0.0;
        }

        const double iou = intersectionArea / unionArea;

        CPLSetErrorHandler(CPLDefaultErrorHandler);

        return iou;
    }

    template < class GeometryType1, class GeometryType2 >
    double iou ( const std::shared_ptr < GeometryType1 > &geom1, const std::shared_ptr < GeometryType2 > &geom2 )
    {
        return iou ( geom1.get(), geom2.get () );
    }
}
