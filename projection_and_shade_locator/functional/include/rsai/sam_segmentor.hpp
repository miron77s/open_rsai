#pragma once

#include "rsai/sam_segmentor.h"

#include <atomic>
#include <mutex>
#include "common/definitions.h"
#include "threading_utils/gdal_iterators.h"
#include "rsai/markup/tile_saver.h"
#include "gdal_utils/create_helpers.h"
#include "eigen_utils/gdal_bridges.h"

template < class ProgressFunc >
bool rsai::sam_segmentor::operator () ( const ProgressFunc &progress_func )
{
    if ( !m_raster || !m_objects_vector )
        return false;

    tile_saver a_tile_saver ( m_store_dir, m_raster );

    threading::dataset_iterator iterator ( m_objects_vector );

    //std::cout << "Creating region coverage...\n";

    std::mutex lock;
    /*gdal::shared_geometry coverage;
    const bool prepared = iterator ( [&] ( gdal::shared_feature feature, const int current_feature_id )
    {
        const OGRGeometry *geometry = feature->GetGeometryRef ();
        if ( geometry != nullptr
                && wkbFlatten ( geometry->getGeometryType()) == wkbPolygon )
        {
            auto object_raster_bbox = a_tile_saver.get_bbox( feature );

            {
                std::scoped_lock lock_guard ( lock );
                if ( coverage )
                    coverage = gdal::shared_geometry ( coverage->Union ( object_raster_bbox.to_polygon().get() ) );
                else
                    coverage = gdal::shared_geometry ( object_raster_bbox.to_polygon()->clone() );
            }
        }
    }
    , progress_func, 1 );

    if ( !prepared )
        return false;

    eigen::gdal_dataset_bridge raster_wrapper ( m_raster );
    auto i2w = raster_wrapper.transform();

    gdal::vector_wrapper wrapper ( "coverage", "coverage", std::cerr );

    if ( is_polygon ( coverage ) )
    {
        auto poly_cover = to_polygon ( coverage );
        poly_cover = transform ( poly_cover, i2w );
        wrapper << poly_cover;
    }

    if ( is_multipolygon( coverage ) )
    {
        auto poly_cover = to_multipolygon ( coverage );
        poly_cover = transform ( poly_cover, i2w );
        wrapper << poly_cover;
    }

    sam_tiles tiles ( m_raster, m_store_dir, coverage, { 800, 800 }, { 600, 600 } );*/

    sam_tiles tiles ( m_raster, m_store_dir );

    //std::cout << "Verifying region coverage...\n";
    const bool verified = iterator ( [&] ( gdal::shared_feature feature, const int current_feature_id )
    {
        const OGRGeometry *geometry = feature->GetGeometryRef ();
        if ( geometry != nullptr
                && wkbFlatten ( geometry->getGeometryType()) == wkbPolygon )
        {
            auto object_raster_bbox = a_tile_saver.get_bbox( feature );

            if ( tiles.verify_or_add ( object_raster_bbox ) )
            {
                std::scoped_lock lock_guard ( lock );
                //std::cout << "\rCreated a tile for feature " << current_feature_id << " with bbox " << object_raster_bbox << '\n';
                //std::cout.flush ();
            }

        }
    }
    , progress_func, 1 );

    if ( !verified )
        return false;

    const std::string segment_script = "python3 ./scripts/sam_binary_segments.py";
    const std::string weights_file = "./weights/sam_vit_h_4b8939.pth";
    const std::string command = segment_script + " " + weights_file + " "
                                + m_store_dir + " " + DEFAULT_SEG_ANY_EDGES_WIDTH_VALUE;

    const int result = system(command.c_str());

    if ( result != 0 )
        return false;

    //std::cout << "Created objectwise segments...\n";
    const bool segmented = iterator ( [&] ( gdal::shared_feature feature, const int current_feature_id )
    {
        static const std::string id_field_name = DEFAULT_OBJECT_ID_FIELD_NAME;
        const int object_index = feature->GetFieldAsInteger ( id_field_name.c_str() );
        const std::string object_index_str = std::to_string ( object_index );

        const OGRGeometry *geometry = feature->GetGeometryRef ();
        if ( geometry != nullptr
                && wkbFlatten ( geometry->getGeometryType()) == wkbPolygon )
        {
            //std::cout << "\robject " << object_index_str;
            auto object_raster_bbox = a_tile_saver.get_bbox( feature );
            auto segments = tiles.segments ( object_raster_bbox );
            gdal::to_wkt_file ( m_store_dir + DEFAULT_OBJECT_WKT_FILE_PREFIX + object_index_str + DEFAULT_SEGMENT_WKT_FILE_EXT, segments );

        }
    }
    , progress_func, 1 );

    return result == 0 && segmented;
}
