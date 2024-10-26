#pragma once

#include "helper.h"

namespace gdal
{
    template < class outstream >
    class create_raster_helper_t:
            public helper_t < outstream >
    {
    public:
        create_raster_helper_t ( const std::string &file_name, const std::string &driver, outstream &log, int width, int height, int bands, GDALDataType type );
        create_raster_helper_t ( gdal::shared_dataset ds, outstream &log );

        virtual shared_dataset              validate ( bool required ) override;
        virtual shared_spatial_reference    srs () const override;
    protected:
        const std::string   _driver;
        const int           _width =    0;
        const int           _height =   0;
        const int           _bands =    0;
        const GDALDataType  _type =     GDT_Unknown;
    }; // class create_helper_t

    template < class outstream >
    class create_vector_helper_t:
            public create_raster_helper_t < outstream >
    {
    public:
        create_vector_helper_t ( const std::string &file_name, const std::string &driver, outstream &log );
        create_vector_helper_t ( gdal::shared_dataset ds, outstream &log );

        virtual shared_spatial_reference    srs () const override;
    protected:
    }; // class create_helper_t

    using create_raster_helper = create_raster_helper_t < std::ostream >;
    using create_vector_helper = create_vector_helper_t < std::ostream >;

    template < class outstream >
    class vector_wrapper_t
    {
    public:
        vector_wrapper_t ( const std::string &file_name, const std::string &layer_name, outstream &log
                                , shared_spatial_reference srs = shared_spatial_reference::from_epsg ( DEFAULT_SPATIAL_REFERENCE_EPSG ) );

        template < class Geometry_Type >
        vector_wrapper_t & operator << ( std::shared_ptr < Geometry_Type >     &geom );

    private:
       shared_dataset   _ds;
       ogr_layer      * _layer;
       shared_feature   _new_feature;
    };

    using vector_wrapper = vector_wrapper_t < std::ostream >;

}; // namespace gdal

#include "create_helpers.hpp"
