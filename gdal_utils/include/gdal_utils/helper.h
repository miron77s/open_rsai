#pragma once

#include <string>
#include <iostream>
#include "shared_dataset.h"
#include "shared_spatial_reference.h"
#include "shared_options.h"
#include "shared_geometry.h"
#include "shared_feature.h"
#include "common/promt_functions.hpp"

namespace gdal
{
    template < class outstream >
    class helper_t
    {
    public:
        helper_t ( const std::string &file_name, outstream &log );
        helper_t ( gdal::shared_dataset ds, outstream &log );

        virtual shared_dataset              validate ( bool required )  = 0;

        shared_dataset                      dataset () const;
        const std::string&                  file_name () const;
        virtual shared_spatial_reference    srs () const                = 0;

        template < class PromtFunc >
        OGRLayer *                          create_layer ( const std::string layer_name, OGRwkbGeometryType geom_type
                                                           , bool force_rewrite, const PromtFunc &promt_func );

        template < class PromtFunc >
        OGRLayer *                          create_layer ( const std::string layer_name, OGRwkbGeometryType geom_type, gdal::shared_options & options
                                                           , bool force_rewrite, const PromtFunc &promt_func );

        template < class PromtFunc >
        OGRLayer *                          create_layer ( const std::string layer_name, OGRwkbGeometryType geom_type, OGRSpatialReference * srs
                                                           , bool force_rewrite, const PromtFunc &promt_func );

        template < class PromtFunc >
        OGRLayer *                          create_layer ( const std::string layer_name, OGRwkbGeometryType geom_type
                                                           , OGRSpatialReference * srs, gdal::shared_options & options
                                                           , bool force_rewrite, const PromtFunc &promt_func );

    protected:
        std::string     _file_name;
        shared_dataset  _ds;
        outstream      &_log;
    }; // class helper_t

    template < class outstream, class helper >
    shared_datasets apply_helper ( const std::vector < std::string > &items, outstream &log );

    template < class outstream, class helper >
    shared_datasets apply_helper ( const std::vector < std::string > &items, const std::string &driver, outstream &log );

    template < class outstream, class helper >
    shared_spatial_references get_srses ( const shared_datasets &datasets, outstream &log );

}; // namespace gdal

#include "helper.hpp"
