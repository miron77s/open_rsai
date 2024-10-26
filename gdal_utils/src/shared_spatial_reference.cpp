#include "gdal_utils/shared_spatial_reference.h"

#include "gdal_utils/shared_geometry.h"

using namespace gdal;

gdal::shared_spatial_reference::shared_spatial_reference ( OGRSpatialReference * p_ref, const std::string &ds_name )
    : std::shared_ptr < OGRSpatialReference > ( p_ref, OGRSpatialReference::DestroySpatialReference ), _ds_name ( ds_name )
{

}

std::string gdal::shared_spatial_reference::export_to_wkt () const
{
    return __to_std_string ( [=] ( char ** p_str ) -> OGRErr { return get ()->exportToWkt ( p_str ); } );
}


std::string gdal::shared_spatial_reference::export_to_pretty_wkt () const
{
    return __to_std_string ( [=] ( char ** p_str ) -> OGRErr { return get ()->exportToPrettyWkt ( p_str ); } );
}

std::string gdal::shared_spatial_reference::dataset_name () const
{
    return _ds_name;
}

shared_spatial_reference gdal::shared_spatial_reference::from_epsg ( const int code )
{
    auto srs = gdal::instance < shared_spatial_reference > ();
    if ( srs->importFromEPSG ( code ) != OGRERR_NONE )
        return nullptr;
    else
        return srs;
}

bool gdal::are_same ( const shared_spatial_references &srses )
{
    if ( srses.size() > 0 )
    {
        const auto &base_srs = srses.front();
        for ( const auto srs : srses )
        {
            if ( !base_srs->IsSame ( srs.get () ) )
                return false;
        }
    }
    return true;
}

bool gdal::operator == ( const shared_spatial_references &lh, const shared_spatial_references &rh )
{
    if ( lh.size() != rh.size() )
        return false;

    for ( int i = 0; i < lh.size(); ++i )
    {
        if ( !lh [i]->IsSame ( rh [i].get() ) )
            return false;
    }

    return true;
}

bool gdal::operator == ( const shared_spatial_references &lh, const shared_spatial_reference &rh )
{
    for ( int i = 0; i < lh.size(); ++i )
    {
        if ( !lh [i]->IsSame ( rh.get() ) )
            return false;
    }

    return true;
}

bool gdal::operator != ( const shared_spatial_references &lh, const shared_spatial_references &rh )
{
    return !( lh == rh );
}

bool gdal::operator != ( const shared_spatial_references &lh, const shared_spatial_reference &rh )
{
    return !( lh == rh );
}
