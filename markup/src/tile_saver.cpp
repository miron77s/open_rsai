#include "rsai/markup/tile_saver.h"

#include <filesystem>

#include "eigen_utils/vector_helpers.h"
#include "eigen_utils/gdal_bridges.h"
#include "eigen_utils/math.hpp"

using namespace rsai;

rsai::tile_saver::tile_saver ( const std::string &path, gdal::shared_dataset raster ):
    m_path ( path ), m_raster ( raster ), m_ds_tile_extractor ( raster )
{
    eigen::gdal_dataset_bridge raster_eigen ( raster );
    m_world_2_raster = raster_eigen.transform ().inverse();

    if ( !std::filesystem::exists ( m_path ) )
        std::filesystem::create_directory ( m_path );
}

gdal::bbox rsai::tile_saver::operator () ( gdal::shared_feature feature, const int tile_buffer_size )
{
    auto raster_box = get_bbox ( feature, tile_buffer_size );

    if ( !raster_box.empty() )
    {
        static const std::string id_field_name = DEFAULT_OBJECT_ID_FIELD_NAME;
        const int object_index = feature->GetFieldAsInteger ( id_field_name.c_str() );
        const std::string object_index_str = std::to_string ( object_index );

        auto tile = m_ds_tile_extractor.roi ( raster_box );
        cv::imwrite ( m_path + object_index_str + DEFAULT_SEGMENT_FILE_EXT, tile );
    }

    return raster_box;
}

gdal::bbox rsai::tile_saver::get_bbox ( gdal::shared_feature feature, const int tile_buffer_size ) const
{
    if ( feature == nullptr || m_raster == nullptr )
        return {};

    const OGRGeometry *geometry = feature->GetGeometryRef ();
    auto polygon = geometry->toPolygon();

    if ( polygon == nullptr )
        return {};

    auto raster_object = gdal::operator * ( polygon, m_world_2_raster );
    gdal::bbox raster_box ( raster_object );

    raster_box.bufferize ( { tile_buffer_size, tile_buffer_size } );

    if ( raster_box.verify ( m_raster ) )
    {
        return raster_box;
    }
    else
        return {};
}
