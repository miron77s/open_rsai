#pragma once

#include <ostream>
#include <filesystem>

#include "gdal_utils/create_helpers.h"

using namespace gdal;

template < class outstream >
create_raster_helper_t < outstream >::create_raster_helper_t ( const std::string &file_name, const std::string &driver, outstream &log
                                                               , int width, int height, int bands, GDALDataType type )
    : helper_t < outstream > ( file_name, log )
    , _driver ( driver )
    , _width ( width )
    , _height ( height )
    , _bands ( bands )
    , _type ( type )
{
}

template < class outstream >
create_raster_helper_t < outstream >::create_raster_helper_t ( gdal::shared_dataset ds, outstream &log )
    : helper_t < outstream > ( ds, log )
    , _driver ( ( ds ) ? ds->GetDriverName() : "" )
    , _width ( ( ds ) ? ds->GetRasterXSize () : 0 )
    , _height ( ( ds ) ? ds->GetRasterYSize () : 0 )
    , _bands ( ( ds ) ? ds->GetRasterCount() : 0 )
    , _type ( ( ds ) ? ( ds->GetRasterCount() ? ds->GetRasterBand( 1 )->GetRasterDataType () : GDT_Unknown ) : GDT_Unknown )
{
}

template < class outstream >
shared_dataset create_raster_helper_t < outstream >::validate ( bool required )
{
    const auto & file_name = helper_t < outstream >::file_name ();

    if ( !std::filesystem::exists ( file_name ) )
        std::filesystem::create_directories ( file_name );

    helper_t < outstream >::_ds = gdal::create_dataset( _driver, file_name, _width, _height, _bands, _type );

    const bool created = ( helper_t < outstream >::_ds != nullptr );
    if ( !created && required )
        helper_t < outstream >::_log << "Driver '" << _driver << "' to create dataset '" << file_name << "' is not found" << std::endl;

    return helper_t < outstream >::dataset ();
}

template < class outstream >
shared_spatial_reference create_raster_helper_t < outstream >::srs () const
{
    return helper_t < outstream >::dataset ().raster_srs ();
}

template < class outstream >
create_vector_helper_t < outstream >::create_vector_helper_t ( const std::string &file_name, const std::string &driver, outstream &log )
    : create_raster_helper_t < outstream > ( file_name, driver, log, 0, 0, 0, GDT_Unknown )
{
}

template < class outstream >
create_vector_helper_t < outstream >::create_vector_helper_t ( gdal::shared_dataset ds, outstream &log )
    : create_raster_helper_t < outstream > ( ds, log )
{
}

template < class outstream >
shared_spatial_reference create_vector_helper_t < outstream >::srs () const
{
    return helper_t < outstream >::dataset ().vector_srs ( 0 );
}

template < class outstream >
vector_wrapper_t < outstream >::vector_wrapper_t ( const std::string &file_name, const std::string &layer_name, outstream &log
                                               , shared_spatial_reference srs )
{
    gdal::create_vector_helper out_helper ( file_name, DEFAULT_VECTOR_MAP_OUTPUT_DRIVER, log );
    _ds = out_helper.validate ( true );
    _layer = out_helper.create_layer ( layer_name, wkbUnknown, srs.get (), true, rewrite_layer_promt_dummy );

    if ( _layer )
        _new_feature = shared_feature ( _layer->GetLayerDefn() );
}

template < class outstream >
template < class Geometry_Type >
vector_wrapper_t < outstream > & vector_wrapper_t < outstream >::operator << ( std::shared_ptr < Geometry_Type > &geom )
{
    if ( _layer && _new_feature )
    {
        _new_feature->SetGeometry ( geom.get ()->clone() );
        if( _layer->CreateFeature( _new_feature.get () ) != OGRERR_NONE )
            std::cerr << "Stop: Failed to create feature in layer " << _layer->GetName() << std::endl;
    }

    return *this;
}
