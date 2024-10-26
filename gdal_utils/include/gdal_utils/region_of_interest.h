#pragma once

#include <mutex>

#include "gdal_utils/shared_dataset.h"
#include "gdal_utils/shared_spatial_reference.h"
#include "gdal_utils/shared_geometry.h"

namespace gdal
{
    class region_of_interest
    {
    public:
        region_of_interest ( const shared_dataset &ds );

        geometry     first_found () const;

    private:
        shared_dataset _ds;
    }; // class region_of_interest

    class layer_simple_reader
    {
    public:
        layer_simple_reader ( ogr_layer * layer );

        geometries operator () ( polygon &bounds );

    private:
        ogr_layer * m_layer;
        std::mutex m_locker;
    }; // class layer_reader

}; // namespace gdal
