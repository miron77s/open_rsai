#include "gdal_utils/shared_feature.h"

#include <ogrsf_frmts.h>

using namespace gdal;

gdal::shared_feature::shared_feature ()
    : std::shared_ptr < element_type > ( nullptr, OGRFeature::DestroyFeature )
{

}

gdal::shared_feature::shared_feature ( OGRFeature * feature )
    : std::shared_ptr < element_type > ( feature, OGRFeature::DestroyFeature )
{

}

gdal::shared_feature::shared_feature ( OGRFeatureDefn * feature_definition )
    : std::shared_ptr < element_type > ( OGRFeature::CreateFeature ( feature_definition ), OGRFeature::DestroyFeature )
{

}

gdal::field_definition::field_definition ( const std::string &name, const OGRFieldType &type
                                               , const int width, const bool nullable )
    : name ( name ), type ( type ), width ( width ), nullable ( nullable )
{

}

OGRLayer * gdal::operator << ( OGRLayer *layer, field_definition fd )
{
    OGRFieldDefn field ( fd.name.c_str (), fd.type );
    if ( fd.width != DEFAULT_FIELD_SIZE )
        field.SetWidth ( fd.width );
    field.SetNullable ( fd.nullable );
    layer->CreateField ( &field );

    return layer;
}
