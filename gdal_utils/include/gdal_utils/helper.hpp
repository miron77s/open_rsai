#pragma once

#include <ostream>
#include "common/definitions.h"
#include "gdal_utils/helper.h"

using namespace gdal;

template < class outstream >
helper_t < outstream >::helper_t ( const std::string &file_name, outstream &log )
    : _file_name ( file_name ), _log ( log )
{
}

template < class outstream >
helper_t < outstream >::helper_t ( gdal::shared_dataset ds, outstream &log )
    : _ds ( ds ), _log ( log )
{
    if ( ds )
        _file_name = ds->GetDescription();
}

template < class outstream >
shared_dataset helper_t < outstream >::dataset () const
{
    return _ds;
}

template < class outstream >
const std::string & helper_t < outstream >::file_name () const
{
    return _file_name;
}

template < class outstream >
template < class PromtFunc >
OGRLayer * helper_t < outstream >::create_layer ( const std::string layer_name, OGRwkbGeometryType geom_type
                                                  , bool force_rewrite, const PromtFunc &promt_func )
{
    // default options
    gdal::shared_options options;
    options.set_encoding        ( DEFAULT_ENCODING );

    return create_layer ( layer_name, geom_type, options, force_rewrite, promt_func );
}

template < class outstream >
template < class PromtFunc >
OGRLayer * helper_t < outstream >::create_layer ( const std::string layer_name, OGRwkbGeometryType geom_type, gdal::shared_options & options
                                                  , bool force_rewrite, const PromtFunc &promt_func )
{
    return create_layer ( layer_name, geom_type, srs().get (), options, force_rewrite, promt_func );
}

template < class outstream >
template < class PromtFunc >
OGRLayer * helper_t < outstream >::create_layer ( const std::string layer_name, OGRwkbGeometryType geom_type, OGRSpatialReference * srs
                                                  , bool force_rewrite, const PromtFunc &promt_func )
{
    // default options
    gdal::shared_options options;
    options.set_encoding        ( DEFAULT_ENCODING );

    return create_layer ( layer_name, geom_type, srs, options, force_rewrite, promt_func );
}

template < class outstream >
template < class PromtFunc >
OGRLayer * helper_t < outstream >::create_layer ( const std::string layer_name, OGRwkbGeometryType geom_type
                                                  , OGRSpatialReference * srs, gdal::shared_options & options
                                                  , bool force_rewrite, const PromtFunc &promt_func )
{
    if ( _ds == nullptr )
    {
        std::cerr << "Failed to create layer '" << layer_name << "' due to null dataset!" << std::endl;
        return nullptr;
    }

    const bool proceed = promt_func ( _ds, layer_name, force_rewrite );

    if ( !proceed )
        return nullptr;

    auto roof_layer = _ds->CreateLayer ( layer_name.c_str (), srs, geom_type, options.get () );

    if ( !roof_layer )
        std::cerr << "Failed to create layer '" << layer_name << "' in dataset '" << _ds->GetDescription () << "'!" << std::endl;
    else
        std::cout << "Created layer '" << layer_name << "' in dataset '" << _ds->GetDescription () << "'." << std::endl;

    return roof_layer;
}

template < class outstream, class helper >
shared_datasets gdal::apply_helper ( const std::vector < std::string > &items, outstream &log )
{
    shared_datasets dses;
    for ( const auto &input_vector : items )
    {
        helper a_helper ( input_vector, std::cerr );
        auto ds = a_helper.validate ( true );
        dses.push_back ( ds );
    }

    return std::move ( dses );
}

template < class outstream, class helper >
shared_datasets gdal::apply_helper ( const std::vector < std::string > &items, const std::string &driver, outstream &log )
{
    shared_datasets ds_vectors;
    for ( const auto &input_vector : items )
    {
        helper a_helper ( input_vector, driver, std::cerr );
        auto ds_vector = a_helper.validate ( true );
        ds_vectors.push_back ( ds_vector );
    }

    return std::move ( ds_vectors );
}

template < class outstream, class helper >
shared_spatial_references gdal::get_srses ( const shared_datasets &datasets, outstream &log )
{
    shared_spatial_references srses;

    for ( const auto &dataset : datasets )
    {
        helper a_helper ( dataset, std::cerr );
        srses.push_back ( a_helper.srs () );
    }

    return std::move ( srses );
}
