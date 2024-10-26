#pragma once

#include "helper.h"

namespace gdal
{
    template < class outstream >
    class open_ro_helper_t:
            public helper_t < outstream >
    {
    public:
        open_ro_helper_t ( const std::string &file_name, outstream &log );
        open_ro_helper_t ( gdal::shared_dataset ds, outstream &log );

        virtual shared_dataset validate ( bool required ) override;
    protected:
        int             _mode = GA_ReadOnly;
    }; // class open_ro_helper_t

    template < class outstream >
    class open_raster_ro_helper_t:
            public open_ro_helper_t < outstream >
    {
    public:
        open_raster_ro_helper_t ( const std::string &raster_name, outstream &log );
        open_raster_ro_helper_t ( gdal::shared_dataset ds, outstream &log );

        virtual shared_dataset              validate ( bool required ) override;
        virtual shared_spatial_reference    srs () const override;
    }; // class raster_open_ro_helper_t

    template < class outstream >
    class open_vector_ro_helper_t:
            public open_ro_helper_t < outstream >
    {
    public:
        open_vector_ro_helper_t ( const std::string &vector_name, outstream &log );
        open_vector_ro_helper_t ( gdal::shared_dataset ds, outstream &log );

        virtual shared_dataset              validate ( bool required ) override;
        virtual shared_spatial_reference    srs () const override;
    }; // class vector_open_ro_helper_t

    template < class outstream >
    class open_raster_rw_helper_t:
            public open_raster_ro_helper_t < outstream >
    {
    public:
        open_raster_rw_helper_t ( const std::string &raster_name, outstream &log );
        open_raster_rw_helper_t ( gdal::shared_dataset ds, outstream &log );
    }; // class raster_open_rw_helper_t

    template < class outstream >
    class open_vector_rw_helper_t:
            public open_vector_ro_helper_t < outstream >
    {
    public:
        open_vector_rw_helper_t ( const std::string &raster_name, outstream &log );
        open_vector_rw_helper_t ( gdal::shared_dataset ds, outstream &log );
    }; // class vector_open_rw_helper_t

    using open_raster_ro_helper = open_raster_ro_helper_t < std::ostream >;
    using open_vector_ro_helper = open_vector_ro_helper_t < std::ostream >;
    using open_raster_rw_helper = open_raster_rw_helper_t < std::ostream >;
    using open_vector_rw_helper = open_vector_rw_helper_t < std::ostream >;
}; // namespace gdal

#include "open_helpers.hpp"
