#pragma once

#include "gdal_utils/shared_spatial_reference.h"

template < class outstream >
bool gdal::shared_spatial_reference::verify ( outstream &out ) const
{
    if ( get () == nullptr )
    {
       out << "SRS from dataset " << _ds_name << " is empty\n";
       return false;
    }

    if ( get ()->IsEmpty() )
    {
       out << "SRS from dataset " << _ds_name << " is empty\n";
       return false;
    }

    if ( get ()->IsLocal() )
    {
       out << "SRS from dataset " << _ds_name << " is not geographic\n";
       out << "Details:\n" << export_to_pretty_wkt();
       return false;
    }

    return true;
}

template < class func, class string_type >
string_type gdal::shared_spatial_reference::__to_std_string ( const func &f ) const
{
    auto srs = get ();
    if ( srs != nullptr )
    {
        char * p_str = nullptr;
        f ( &p_str );

        std::string wkt = p_str;
        CPLFree ( p_str );

        return wkt;
    }
    else
        return std::string ();
}
