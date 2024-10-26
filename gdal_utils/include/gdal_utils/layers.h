#pragma once

#include <vector>
#include <string>
#include <mutex>
#include "shared_geometry.h"
#include "shared_feature.h"
#include "shared_dataset.h"

namespace gdal
{
    class ogr_layers
    {
    public:
        void            add ( ogr_layer* layer );
        void            set_spatial_filter ( OGRGeometry* geometry );
        void            set_attribute_filter ( const std::string &query );
        shared_features search ();
        void            reset_reading ();

    private:
        std::vector < ogr_layer* > m_layers;
        std::vector < std::mutex > m_locks;
    };
}
