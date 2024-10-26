#pragma once

#include <mutex>
#include "gdal_utils/shared_feature.h"

namespace threading
{
    class shared_layer_iterator
    {
    public:
        shared_layer_iterator (OGRLayer* layer);
        shared_layer_iterator ( const shared_layer_iterator &src );
        gdal::shared_feature    next_feature();
        void                    set_feature ( gdal::shared_feature new_feature );
        void                    reset ();
        OGRLayer*               layer ();
        gdal::shared_features   find_feature ( const std::string &filter );
    private:
        OGRLayer* m_layer;
        std::mutex m_mutex;
    };
}; // namespace threading
