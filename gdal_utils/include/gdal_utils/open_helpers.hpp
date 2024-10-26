#pragma once

#include <ostream>

#include "gdal_utils/open_helpers.h"

using namespace gdal;

template < class outstream >
open_ro_helper_t < outstream >::open_ro_helper_t ( const std::string &file_name, outstream &log )
    : helper_t < outstream > ( file_name, log )
{
}

template < class outstream >
open_ro_helper_t < outstream >::open_ro_helper_t ( gdal::shared_dataset ds, outstream &log )
    : helper_t < outstream > ( ds, log )
{
}

template < class outstream >
shared_dataset open_ro_helper_t < outstream >::validate ( bool required )
{
    helper_t < outstream >::_ds = gdal::open_dataset ( helper_t < outstream >::file_name (), _mode );

    const bool opened = ( helper_t < outstream >::_ds != nullptr );
    if ( !opened && ( required || !helper_t < outstream >::file_name ().empty () ) )
        helper_t < outstream >::_log << "Dataset '" << helper_t < outstream >::file_name () << "' cannot be opened" << std::endl;  // TODO - add mode naming

    return helper_t < outstream >::dataset ();
}

template < class outstream >
open_raster_ro_helper_t < outstream >::open_raster_ro_helper_t ( const std::string &file_name, outstream &log )
    : open_ro_helper_t < outstream > ( file_name, log )
{
}

template < class outstream >
open_raster_ro_helper_t < outstream >::open_raster_ro_helper_t ( gdal::shared_dataset ds, outstream &log )
    : open_ro_helper_t < outstream > ( ds, log )
{
}

template < class outstream >
shared_dataset open_raster_ro_helper_t < outstream >::validate ( bool required )
{
    const auto ds = open_ro_helper_t < outstream >::validate ( required );
    if ( ds == nullptr )
        return ds;

    const int bands = helper_t < outstream >::_ds->GetRasterCount ();

    if ( bands == 0 )
    {
        helper_t < outstream >::_log << "Dataset '" << helper_t < outstream >::file_name () << "' does not have any raster bands" << std::endl;
        helper_t < outstream >::_ds = nullptr;
        return helper_t < outstream >::dataset ();
    }

    const int width = helper_t < outstream >::_ds->GetRasterXSize (),
              height = helper_t < outstream >::_ds->GetRasterYSize ();

    if ( width == 0 || height == 0 )
    {
        helper_t < outstream >::_log << "Dataset " << helper_t < outstream >::file_name ()
                                     << " has invalid sizes " << width << ", " << height << std::endl;
        helper_t < outstream >::_ds = nullptr;
        return helper_t < outstream >::dataset ();
    }

    return helper_t < outstream >::dataset ();
}

template < class outstream >
shared_spatial_reference open_raster_ro_helper_t < outstream >::srs () const
{
    return helper_t < outstream >::dataset ().raster_srs ();
}

template < class outstream >
open_vector_ro_helper_t < outstream >::open_vector_ro_helper_t ( const std::string &file_name, outstream &log )
    : open_ro_helper_t < outstream > ( file_name, log )
{
}

template < class outstream >
open_vector_ro_helper_t < outstream >::open_vector_ro_helper_t ( gdal::shared_dataset ds, outstream &log )
    : open_ro_helper_t < outstream > ( ds, log )
{
}

template < class outstream >
shared_dataset open_vector_ro_helper_t < outstream >::validate ( bool required )
{
    const auto ds = open_ro_helper_t < outstream >::validate ( required );
    if ( ds == nullptr )
        return ds;

    const int layers = helper_t < outstream >::_ds->GetLayerCount ();

    if ( layers == 0 )
    {
        helper_t < outstream >::_log << "Dataset '" << helper_t < outstream >::file_name () << "' does not have any vector layers" << std::endl;
        helper_t < outstream >::_ds = nullptr;
        return helper_t < outstream >::dataset ();
    }

    return helper_t < outstream >::dataset ();
}

template < class outstream >
shared_spatial_reference open_vector_ro_helper_t < outstream >::srs () const
{
    return helper_t < outstream >::dataset ().vector_srs ( 0 );
}

template < class outstream >
open_raster_rw_helper_t < outstream >::open_raster_rw_helper_t ( const std::string &file_name, outstream &log )
    : open_raster_rw_helper_t < outstream > ( file_name, log )
{
    open_ro_helper_t < outstream >::_mode = GA_Update;
}

template < class outstream >
open_raster_rw_helper_t < outstream >::open_raster_rw_helper_t ( gdal::shared_dataset ds, outstream &log )
    : open_raster_rw_helper_t < outstream > ( ds, log )
{
    open_ro_helper_t < outstream >::_mode = GA_Update;
}

template < class outstream >
open_vector_rw_helper_t < outstream >::open_vector_rw_helper_t ( const std::string &file_name, outstream &log )
    : open_vector_rw_helper_t < outstream > ( file_name, log )
{
    open_ro_helper_t < outstream >::_mode = GA_Update;
}

template < class outstream >
open_vector_rw_helper_t < outstream >::open_vector_rw_helper_t ( gdal::shared_dataset ds, outstream &log )
    : open_vector_rw_helper_t < outstream > ( ds, log )
{
    open_ro_helper_t < outstream >::_mode = GA_Update;
}
