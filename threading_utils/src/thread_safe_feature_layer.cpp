#include "threading_utils/thread_safe_feature_layer.h"

#include <iostream>

using namespace threading;

shared_layer_iterator::shared_layer_iterator(OGRLayer* layer)
    :m_layer ( layer )
{    
    if ( m_layer )
        m_layer->ResetReading();
}

shared_layer_iterator::shared_layer_iterator ( const shared_layer_iterator &src )
    : m_layer ( src.m_layer )
{

}

gdal::shared_feature shared_layer_iterator::next_feature()
{
    if ( m_layer )
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return {m_layer->GetNextFeature()};
    }
    else
        return gdal::shared_feature ();
}

void shared_layer_iterator::set_feature ( gdal::shared_feature new_feature )
{
    if ( m_layer )
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if( m_layer->CreateFeature( new_feature.get () ) != OGRERR_NONE )
        {
            std::cerr << "Stop: Failed to create feature in layer " << m_layer->GetName () << std::endl;
        }
    }
}

void shared_layer_iterator::reset ()
{
    if ( m_layer )
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_layer->ResetReading();
    }
}

OGRLayer* shared_layer_iterator::layer ()
{
    return m_layer;
}

gdal::shared_features shared_layer_iterator::find_feature ( const std::string &filter )
{
    if ( m_layer )
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_layer->ResetReading();
        m_layer->SetAttributeFilter ( filter.c_str() );

        gdal::shared_features result;
        while ( auto feature = gdal::shared_feature ( m_layer->GetNextFeature() ) )
        {
            result.push_back ( feature );
        }

        return result;
    }

    return {};
}
