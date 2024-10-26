#include "gdal_utils/region_of_interest.h"

#include <ogrsf_frmts.h>

using namespace gdal;

gdal::region_of_interest::region_of_interest ( const shared_dataset &ds  )
    : _ds ( ds )
{
}

gdal::geometry gdal::region_of_interest::first_found () const
{
    if ( _ds != nullptr )
    {
        for( auto layer: _ds->GetLayers() )
        {
            for( const auto& feature: *layer )
            {
                const OGRGeometry *geometry = feature->GetGeometryRef();
                if( geometry != nullptr
                        && wkbFlatten(geometry->getGeometryType()) == wkbPolygon )
                {
                    gdal::geometry roi ( geometry->clone () );
                    return roi;
                }
            }
        }
    }

    return {};
}

gdal::layer_simple_reader::layer_simple_reader ( ogr_layer * layer )
    : m_layer ( layer )
{
}

geometries gdal::layer_simple_reader::operator () ( polygon &bounds )
{
    std::scoped_lock lock ( m_locker );

    geometries result;
    if ( m_layer )
    {
        m_layer->SetSpatialFilter ( bounds.get() );
        m_layer->ResetReading();

        while ( auto feature = m_layer->GetNextFeature() )
        {
            result.emplace_back ( feature->GetGeometryRef()->clone() );
        }
    }
    return std::move ( result );
}
