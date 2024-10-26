#include "gdal_utils/layers.h"

using namespace gdal;

void gdal::ogr_layers::add ( ogr_layer* layer )
{
    m_layers.push_back ( layer );
    //m_locks.resize ( m_layers.size() );
}

void gdal::ogr_layers::set_spatial_filter ( OGRGeometry* geometry )
{
    for ( int i = 0; i < m_layers.size(); ++i )
    {
        OGRLayer* layer = m_layers [i];
        //std::scoped_lock lock ( m_locks [i] );
        layer->SetSpatialFilter ( geometry );
    }
}

void gdal::ogr_layers::set_attribute_filter ( const std::string &query )
{
    for ( int i = 0; i < m_layers.size(); ++i )
    {
        OGRLayer* layer = m_layers [i];
        //std::scoped_lock lock ( m_locks [i] );
        layer->SetAttributeFilter ( query.c_str () );
    }
}

shared_features gdal::ogr_layers::search ()
{
    reset_reading ();

    shared_features matching_features;
    for ( int i = 0; i < m_layers.size(); ++i )
    {
        OGRLayer* layer = m_layers [i];
        //std::scoped_lock lock ( m_locks [i] );
        shared_feature feature;
        while ( ( feature = layer->GetNextFeature() ) != nullptr)
            matching_features.push_back(feature);
    }
    return matching_features;
}

void gdal::ogr_layers::reset_reading()
{
    for ( int i = 0; i < m_layers.size(); ++i )
    {
        OGRLayer* layer = m_layers [i];
        //std::scoped_lock lock ( m_locks [i] );
        layer->ResetReading();
    }
}
