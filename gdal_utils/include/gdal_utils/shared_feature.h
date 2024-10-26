#pragma once

#include <memory>
#include <string>
#include <ogrsf_frmts.h>

#include "common/definitions.h"

namespace gdal
{
    class shared_feature:
            public std::shared_ptr < OGRFeature >
    {
    public:
        shared_feature ();
        shared_feature ( OGRFeature * feature );
        shared_feature ( OGRFeatureDefn * feature_definition );
    }; // class shared_feature

    using shared_features = std::vector < shared_feature >;

    struct field_definition
    {
        field_definition ( const std::string &name, const OGRFieldType &type
                         , const int width = DEFAULT_FIELD_SIZE, const bool nullable = DEFAULT_FIELD_CAN_BE_NULL );

        std::string name;
        OGRFieldType type   = OFTString;
        int width           = DEFAULT_FIELD_SIZE;
        bool nullable       = DEFAULT_FIELD_CAN_BE_NULL;
    }; // class field_string

    OGRLayer * operator << ( OGRLayer *layer, field_definition fd );
} // namespace gdal
