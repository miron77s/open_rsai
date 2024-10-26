#pragma once

#include "rsai/segany_markup_by_objects.h"

#include <ogrsf_frmts.h>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <cstdlib>

#include "common/definitions.h"
#include "gdal_utils/all_helpers.h"
#include "eigen_utils/vector_helpers.h"
#include "eigen_utils/gdal_bridges.h"
#include "eigen_utils/geometry.h"
#include "eigen_utils/math.hpp"
#include "gdal_utils/shared_options.h"
#include "gdal_utils/shared_feature.h"
#include "opencv_utils/raster_roi.h"
#include "opencv_utils/gdal_bridges.h"
#include "threading_utils/gdal_iterators.h"
#include "geometry_utils/points_fill.h"
#include "rsai/markup/tile_saver.h"

using namespace rsai;

template < class PromtFunc, class LayerPromtFunc, class ProgressFunc >
rsai::segany_markup_by_objects::segany_markup_by_objects (
                                                             gdal::shared_dataset roof_vector
                                                           , gdal::shared_dataset raster
                                                           , gdal::shared_dataset markup_map
                                                           , const std::string &markup_directory
                                                           , int tile_buffer_size
                                                           , const bool force_rewrite
                                                           , const PromtFunc &promt_func
                                                           , const LayerPromtFunc &layer_promt_func
                                                           , const ProgressFunc &progress_func
                                                          )
{
    const std::string id_field_name = DEFAULT_OBJECT_ID_FIELD_NAME;

    // Obtain raster-2-world and vice versa transforms
    eigen::gdal_dataset_bridge raster_eigen ( raster );
    Eigen::Matrix3d raster_2_world = raster_eigen.transform ();
    Eigen::Matrix3d world_2_raster = raster_2_world.inverse ();

    const int layers = roof_vector->GetLayerCount ();

    const bool proceed = promt_func ( markup_directory, force_rewrite );

    if ( !proceed )
        return;

    std::filesystem::create_directories ( markup_directory );

    bool layers_result = true;

    for( int i = 0; i < layers; ++i )
    {
        auto layer = roof_vector->GetLayer ( i );

        std::string layer_name = DEFAULT_MARKUP_LAYER_NAME;

        OGRLayer * markup_layer = nullptr;

        if ( markup_map != nullptr )
        {
            const std::string map_dir = markup_map->GetDescription();
            std::filesystem::create_directories(map_dir);

            // creating options
            gdal::shared_options options;
            options.set_encoding        ( DEFAULT_ENCODING );
            const bool proceed_layer = layer_promt_func ( markup_map, layer_name, force_rewrite );
            markup_layer = ( proceed_layer )
                    ? markup_map->CreateLayer ( layer_name.c_str (), roof_vector.vector_srs( i ).get (), wkbMultiPoint, options.get () )
                    : nullptr;
        }

        const float features_count = layer->GetFeatureCount ();
        std::atomic_int features_processed ( 0 );

//        opencv::dataset_roi_extractor ds_tile_extractor ( raster );

        threading::shared_layer_iterator source_layer_iter ( layer );
        threading::shared_layer_iterator markup_layer_iter ( markup_layer );

        tile_saver a_tile_saver ( markup_directory + "/", raster );

        threading::layer_iterator a_layer_iterator ( layer );
        layers_result &= a_layer_iterator ( [&] ( gdal::shared_feature feature, const int current_feature_id )
        {
            const OGRGeometry *geometry = feature->GetGeometryRef ();
            if ( geometry != nullptr
                    && wkbFlatten ( geometry->getGeometryType()) == wkbPolygon )
            {
                gdal::shared_feature markup_feature;
                if ( markup_layer != nullptr )
                    markup_feature = gdal::shared_feature ( markup_layer->GetLayerDefn() );

                auto polygon = geometry->toPolygon();

                if ( polygon == nullptr )
                    return;

                const int object_index = feature->GetFieldAsInteger ( id_field_name.c_str () );
                const std::string object_index_str = std::to_string ( object_index );

                auto object_raster_bbox = a_tile_saver.get_bbox ( feature );
                auto bufferized_raster_box = a_tile_saver ( feature, tile_buffer_size );

                geometry::point_fill markup_creator ( polygon );
                auto object_markup = markup_creator ();

                if ( markup_feature )
                {
                    markup_feature->SetGeometry ( object_markup.positive.get () );
                    markup_layer_iter.set_feature ( markup_feature );
                }

                auto positive_local = gdal::operator * ( object_markup.positive, world_2_raster );
                auto positive_tile = gdal::operator + ( positive_local, -bufferized_raster_box.top_left() );

                /*auto negative_local = gdal::transform ( object_markup.negative, world_2_raster );
                auto negative_tile = gdal::shift ( negative_local, -raster_box.top_left() );*/

                std::ofstream positive_writer ( markup_directory + "/" + object_index_str + ".points" );
                positive_writer << positive_tile->exportToWkt();
                positive_writer.close ();

                object_raster_bbox += -bufferized_raster_box.top_left();
                std::ofstream bbox_writer ( markup_directory + "/" + object_index_str + ".bbox" );
                bbox_writer << object_raster_bbox;
                bbox_writer.close ();

                auto raster_object = gdal::operator * ( polygon, world_2_raster );
                raster_object = gdal::operator + ( raster_object, -bufferized_raster_box.top_left() );
                std::ofstream object_writer ( markup_directory + "/" + object_index_str + ".object" );
                object_writer << raster_object->exportToWkt();
                object_writer.close ();

                /*std::ofstream negative_writer ( markup_directory + "/" + object_index_str + ".negative" );
                negative_writer << negative_tile->exportToWkt();
                negative_writer.close ();*/

                std::ofstream shift_writer ( markup_directory + "/" + object_index_str + ".shift" );
                shift_writer << bufferized_raster_box.top_left() [0] << " " << bufferized_raster_box.top_left() [1];
                shift_writer.close ();
            }
        } , i, layers, progress_func );

        progress_func ( i + 1, layers, 1.0f, true );
    }

    const std::string segment_script = "python3 ./scripts/sam_locate_object.py";
    const std::string weights_file = "/home/miron/projects/Segment\\ Anything/segment-anything/checkpoints/sam_vit_h_4b8939.pth";
    const std::string command = segment_script + " " + weights_file + " " + markup_directory;

    const int result = system(command.c_str());
}

