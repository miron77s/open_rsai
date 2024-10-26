#pragma once

#include "rsai/projection_and_shade_locator.h"

#include <ogrsf_frmts.h>
#include <cstdint>

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
#include "opencv_utils/geometry_renderer.h"
#include "threading_utils/gdal_iterators.h"
#include "rsai/building_models/prismatic.h"
#include "rsai/building_models/roof_estimator.h"
#include "rsai/building_models/structure_estimator.h"
#include "differentiation/convolution_mask.h"
#include "differentiation/gauss_directed_derivative.h"
#include "differentiation/edge_detection.h"
#include "rsai/building_variants_saver.h"
#include "rsai/building_models/building_renderer.h"
#include "rsai/sam_segmentor.h"

using namespace rsai;

struct model_details
{
    Eigen::Vector2d shift;
    int length = 0;
    rsai::building_models::prismatic model;
    double estimate = 0.0;

    model_details ( const model_details & ) = default;

    model_details & operator = ( model_details && from ) { std::swap ( shift, from.shift ); length = from.length;
                                                           std::swap ( model, from.model ); estimate = from.estimate; return *this; }
    bool operator > ( const model_details &rh ) const { return estimate > rh.estimate; }
};

struct tile_info
{
    cv::Mat tile;
    cv::Mat tile_marked;
    Eigen::Vector2d top_left;
    std::string index;
    gdal::polygon original_object;
};

template < class PromtFunc, class ProgressFunc >
rsai::projection_and_shade_locator::projection_and_shade_locator (
                                                                     gdal::shared_dataset &ds_vector
                                                                   , gdal::shared_dataset &ds_raster
                                                                   , gdal::shared_dataset &ds_out
                                                                   , const double segmentize_step
                                                                   , const int projection_step
                                                                   , const double roof_position_walk
                                                                   , const int roof_variants
                                                                   , const int shade_variants
                                                                   , const bool force_rewrite
                                                                   , const bool use_sam
                                                                   , run_mode a_run_mode
                                                                   , interaction_mode an_interaction_mode
                                                                   , const PromtFunc &promt_func
                                                                   , const ProgressFunc &progress_func
                                                                  )
{
    static const std::string id_field_name = DEFAULT_OBJECT_ID_FIELD_NAME;

    // Obtain raster-2-world and vice versa transforms
    eigen::gdal_dataset_bridge raster_eigen ( ds_raster );
    Eigen::Matrix3d raster_2_world = raster_eigen.transform ();
    Eigen::Matrix3d world_2_raster = raster_2_world.inverse ();

    const std::string dst_dir = std::string ( ds_out->GetDescription() ) + "/" + DEFAULT_SEGMENTS_DIRECTORY + "/";
    sam_segmentor segmentor ( ds_raster, ds_vector, dst_dir );
    //if ( use_sam )
    {
        std::cout << "Creating Segment anything objects' boundaries...\n";
        if ( !segmentor ( progress_func ) )
            //return;
            ;
    }

    const int layers = ds_vector->GetLayerCount ();

    std::cout << "Reconstructing buildings...\n";

    bool layers_result = true;
    for( int i = 0; i < layers; ++i )
    {
        auto layer = ds_vector->GetLayer ( i );

        // Creating output roof, proj and shade layers
        gdal::create_vector_helper layers_helper ( ds_out, std::cerr );
        auto roof_layer = layers_helper.create_layer ( DEFAULT_ROOF_LAYER_NAME, wkbMultiPolygon, layer->GetSpatialRef(), force_rewrite, promt_func );
        if ( !roof_layer )
            continue;

        auto proj_layer = layers_helper.create_layer ( DEFAULT_PROJECTION_LAYER_NAME, wkbMultiPolygon, layer->GetSpatialRef(), force_rewrite, promt_func );
        if ( !proj_layer )
            continue;

        auto shade_layer = layers_helper.create_layer ( DEFAULT_SHADE_LAYER_NAME, wkbMultiPolygon, layer->GetSpatialRef(), force_rewrite, promt_func );
        if ( !shade_layer )
            continue;

        const float features_count = layer->GetFeatureCount ();
        std::atomic_int features_processed ( 0 );

        // Layers' features thead-safe access
        threading::shared_layer_iterator source_layer_iter ( layer );
        threading::shared_layer_iterator roof_layer_iter ( roof_layer );
        threading::shared_layer_iterator proj_layer_iter ( proj_layer );
        threading::shared_layer_iterator shade_layer_iter ( shade_layer );

        std::atomic<int> file_counter(0);

        opencv::dataset_roi_extractor ds_tile_extractor ( ds_raster );
        rsai::building_variants_saver saver ( std::string ( ds_out->GetDescription() ) + "/" + DEFAULT_VARIANTS_DIRECTORY );
        std::vector < std::pair < tile_info, rsai::building_models::structures > > positions_list;

        threading::layer_iterator a_layer_iterator ( layer );
        layers_result &= a_layer_iterator ( [&] ( gdal::shared_feature feature, const int current_feature_id )
        {
            const OGRGeometry *geometry = feature->GetGeometryRef ();
            if ( geometry != nullptr
                    && wkbFlatten ( geometry->getGeometryType()) == wkbPolygon )
            {
                const int object_index = feature->GetFieldAsInteger ( id_field_name.c_str() );
                const std::string object_index_str = std::to_string ( object_index );

                auto polygon = geometry->toPolygon();

                if ( polygon == nullptr )
                    return;

                // Getting bbox and polygon
                auto bounds_ring = polygon->getExteriorRing ();
                auto object = polygon->getInteriorRing ( 0 );

                if ( bounds_ring == nullptr || object == nullptr )
                    return;

                // Extracting a desired object's tile from source raster
                auto tile_bounds = instance < gdal::polygon > ();
                tile_bounds->addRingDirectly ( bounds_ring->clone () );
                auto tile_bbox  = ds_tile_extractor.raster_bbox ( tile_bounds );
                auto tile       = ds_tile_extractor.roi ( tile_bbox );

                // Loading objects' edges maps
                //cv::Mat edges = cv::Mat::ones( tile.size (), CV_8U );

                gdal::polygons segments;
                if ( use_sam )
                {
                    //edges = cv::imread ( dst_dir + object_index_str + "_edges.jpg" );

                    cv::imwrite ( dst_dir + object_index_str + "_tile.jpg", tile );

                    segments = to_polygon ( gdal::from_wkt_file ( dst_dir + DEFAULT_OBJECT_WKT_FILE_PREFIX + object_index_str + ".wkt" ) );

//                    if ( tile.size () != edges.size () )
//                        std::cerr << "Stop: Image tile and edges size missmatch! "
//                                  << "Tile: " << tile.size () << " edges: " << edges.size () << '\n';
                }

                // Projecting and shading vectors
                cv::Mat tile_gray;
                cv::cvtColor ( tile, tile_gray, cv::COLOR_BGR2GRAY );

                Eigen::Vector2d proj_world ( feature->GetFieldAsDouble ( DEFAULT_PROJ_STEP_X_FIELD_NAME )
                                           , feature->GetFieldAsDouble ( DEFAULT_PROJ_STEP_Y_FIELD_NAME ) );

                Eigen::Vector2d shade_world ( feature->GetFieldAsDouble ( DEFAULT_SHADE_STEP_X_FIELD_NAME )
                                            , feature->GetFieldAsDouble ( DEFAULT_SHADE_STEP_Y_FIELD_NAME ) );

                const double L1_norm = std::fmax (
                                                    std::fmax ( std::fabs ( proj_world [0] ),  std::fabs ( proj_world [1] ) )
                                                  , std::fmax ( std::fabs ( shade_world [0] ), std::fabs ( shade_world [1] ) )
                                                 );
                proj_world  = proj_world / L1_norm;
                shade_world = shade_world / L1_norm;

                const double max_length = feature->GetFieldAsDouble ( DEFAULT_VECTOR_MAX_LENGTH_NAME );

                // Creating a building model
                building_models::prismatic model ( object, proj_world, shade_world );

                const int current_file_index = ++file_counter;

                auto roof = instance < gdal::polygon > ();
                roof->addRingDirectly ( object->clone() );

                // Estimating roof position
                rsai::building_models::roof_estimator roof_estimator ( roof_position_walk, world_2_raster, proj_world, shade_world, max_length, tile_gray );

                auto responses = ( use_sam ) ? roof_estimator ( roof, -tile_bbox.top_left(), segments, roof_variants, dst_dir + object_index_str + "_" )
                                             : roof_estimator ( roof, -tile_bbox.top_left(), roof_variants );

                cv::imwrite ( dst_dir + object_index_str + "_heat.jpg", roof_estimator.heatmap() );

                // Estimating building's structure
                rsai::building_models::structure_estimator structure_estimator ( model, responses, tile_gray, tile_bbox.top_left(), world_2_raster, segmentize_step, segments );
                auto position_estimates = structure_estimator ( max_length, projection_step, roof_variants, shade_variants );

                if ( an_interaction_mode == interaction_mode::external )
                    saver.write ( object_index_str, position_estimates, tile, tile_bbox.top_left(), raster_2_world );

                {
                    static std::mutex mutex;
                    std::lock_guard < std::mutex > lock ( mutex );

                    positions_list.push_back ( { { tile, tile, tile_bbox.top_left(), object_index_str }, std::move ( position_estimates ) } );
                }
            }
        } , i, layers, progress_func );

        if ( an_interaction_mode == interaction_mode::internal )
        {

            std::cout << "Result reviewing...\n";

            struct map_item
            {
                gdal::multipolygon roof;
                gdal::multipolygon proj;
                gdal::multipolygon shade;
            };

            std::vector < map_item > map_items ( positions_list.size () );

            int key = 0;

            gdal::shared_feature new_feature ( roof_layer_iter.layer()->GetLayerDefn() );
            for ( int j = 0; j < positions_list.size (); )
            {
                auto &positions_data = positions_list [j];
                const auto &tile_tl = positions_data.first.top_left;
                auto &positions = positions_data.second;
                auto obj_id = positions_data.first.index;

                if ( positions.size() == 0 )
                {
                    std::cout << "Skipped feature " << obj_id << " with no positions\n";
                }
                else
                {

                    bool save = false;
                    bool draw_model = true;
                    int current_roof_index = 0;
                    int current_shade_index = 0;
                    cv::namedWindow("Image", cv::WINDOW_AUTOSIZE);

                    while (true && a_run_mode != run_mode::automatic )
                    {

                        std::cout << "Reveiwing " << obj_id << " with positions count " << positions.size() << "\n";
                        std::cout.flush();


                        auto tile = positions_data.first.tile.clone ();

                        if ( draw_model )
                        {
                            auto & local_model = positions[current_roof_index].shades [current_shade_index].model;
                            auto model_geometries = local_model.get();

                            building_renderer renderer ( tile );

                            renderer.render_roof ( model_geometries [0] );
                            renderer.render_projection ( model_geometries [1] );
                            renderer.render_shade ( model_geometries [2] );

                            tile = renderer.get ();
                        }

                        cv::imshow("Image", tile);
                        key = cv::waitKey(0);

                        if (key == 27 || key == 87 || key == 8)
                            break;
                        else if (key == 13)
                        {
                            save = true;
                            break;
                        }
                        else if (key == 81 || key == 83)
                        {
                            if (key == 81 && current_roof_index > 0)
                                --current_roof_index;
                            else if (key == 83 && current_roof_index < positions.size() - 1)
                                ++current_roof_index;
                        }
                        else if (key == 82 || key == 84)
                        {
                            if (key == 84 && current_shade_index > 0)
                                --current_shade_index;
                            else if (key == 82 && current_shade_index < positions[current_roof_index].shades.size() - 1)
                                ++current_shade_index;
                        }
                        else if ( key == 32 )
                            draw_model = !draw_model;
                    }

                    if (key == 87)
                        break;

                    if ( save || a_run_mode == run_mode::automatic )
                    {
                        if ( a_run_mode == run_mode::automatic )
                        {
                            current_roof_index = 0;
                            current_shade_index = 1;
                        }

                        auto & local_model = positions[current_roof_index].shades [current_shade_index].model;
                        local_model.transform_2_world ( raster_2_world, tile_tl );

                        auto & item = map_items [j];
                        item.roof = local_model.roof ();
                        item.proj = local_model.projection();
                        item.shade = local_model.shade ();

                        //map_items.push_back ( { local_model.roof (), local_model.projection(), local_model.shade () } );

                    }
                }

                if (key == 8)
                    --j;
                else
                    ++j;

                progress_func ( i + 1, layers, j / features_count );
            }

            progress_func ( i + 1, layers, 1.0f, true );
            std::cout << "Saving results...";


            for ( const auto &item : map_items )
            {
                if ( item.roof != nullptr )
                {
                    new_feature->SetGeometry ( item.roof.get () );
                    roof_layer_iter.set_feature( new_feature );

                    new_feature->SetGeometry ( item.proj.get () );
                    proj_layer_iter.set_feature( new_feature );

                    new_feature->SetGeometry ( item.shade.get () );
                    shade_layer_iter.set_feature( new_feature );
                }
            }
        }

        std::cout << "done\n";
    }
}

