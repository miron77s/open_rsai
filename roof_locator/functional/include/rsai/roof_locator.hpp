#pragma once

#include "rsai/roof_locator.h"

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
#include "gdal_utils/region_of_interest.h"
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

template < class PromtFunc, class ProgressFunc >
rsai::roof_locator::roof_locator (
                                     gdal::shared_dataset &ds_vector
                                   , gdal::shared_dataset &ds_raster
                                   , gdal::shared_dataset &ds_out
                                   , const double segmentize_step
                                   , const int projection_step
                                   , const double roof_position_walk
                                   , const int roof_variants
                                   , const double min_first_pos_weight
                                   , const double max_first_pos_deviation
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

    gdal::open_vector_ro_helper iv_helper ( ds_vector->GetDescription(), std::cerr );
    auto ds_vector_4_render = iv_helper.validate ( true );

    const std::string dst_dir = std::string ( ds_out->GetDescription() ) + "/" + DEFAULT_SEGMENTS_DIRECTORY + "/";
    sam_segmentor segmentor ( ds_raster, ds_vector, dst_dir );
    if ( use_sam )
    {
        std::cout << "Creating Segment anything objects' boundaries...\n";
        if ( !segmentor ( progress_func ) )
            //return;
            ;
    }

    const int layers = ds_vector->GetLayerCount ();

    std::cout << "Locating roofs...\n";

    bool layers_result = true;
    for( int i = 0; i < layers; ++i )
    {
        auto layer = ds_vector->GetLayer ( i );
        auto layer_4_render = ds_vector_4_render->GetLayer ( i );

        const float features_count = layer->GetFeatureCount ();
        std::atomic_int features_processed ( 0 );

        // Layers' features thead-safe access
        threading::shared_layer_iterator source_layer_iter ( layer );

        std::atomic<int> file_counter(0);

        opencv::dataset_roi_extractor ds_tile_extractor ( ds_raster );
        gdal::layer_simple_reader reader_4_render ( layer_4_render );
        rsai::building_variants_saver saver ( std::string ( ds_out->GetDescription() )
                                              + "/" + DEFAULT_VARIANTS_DIRECTORY + "/" + DEFAULT_ROOFS_SUBDIRECTORY );
        std::vector < std::pair < tile_info, rsai::building_models::roof_responses > > positions_list;

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

                auto geometry_inliers = gdal::to_polygon ( reader_4_render ( tile_bounds ) );
                geometry_inliers *= world_2_raster;
                geometry_inliers += -tile_bbox.top_left();

                cv::Mat tile_marked;
                {
                    building_renderer renderer ( tile, 1 );
                    for ( auto inlier : geometry_inliers )
                        renderer.render_position( inlier, cv::Scalar ( 0x00, 0xFF, 0x00 ), false );

                    tile_marked = renderer.get ();
                }

                // Loading objects' edges maps
                //cv::Mat edges = cv::Mat::ones( tile.size (), CV_8U );

                gdal::polygons segments;
                if ( use_sam )
                {
                    //edges = cv::imread ( dst_dir + object_index_str + "_edges.jpg" );

                    cv::imwrite ( dst_dir + object_index_str + "_tile.jpg", tile_marked );

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

                if ( an_interaction_mode == interaction_mode::external )
                    saver.write ( object_index_str, roof, responses, tile, tile_bbox.top_left(), raster_2_world );
                else
                {
                    static std::mutex mutex;
                    std::lock_guard < std::mutex > lock ( mutex );

                    positions_list.push_back ( { { tile, tile_marked, tile_bbox.top_left(), object_index_str, roof }, std::move ( responses ) } );
                }
            }
        } , i, layers, progress_func );

        if ( an_interaction_mode == interaction_mode::internal )
        {

            std::cout << "Result reviewing...\n";

            const std::string out_dir = ds_out->GetDescription();
            std::ofstream choice_hist ( out_dir + "roof_stats.txt" );
            std::ofstream choice_time ( out_dir + "timings.txt" );
            choice_time << "ID" << '\t' << "index" << '\t' << "actions"
                        << '\t' << "time, s" << "\t" << "size" << "\t" << "response"
                        << "\t" << "best response" << '\t' << "deviation" << '\t' << "best deviation" << '\n';

            double total_time = 0.0;

            std::map < int, int > choice_index_hist;

            // Creating output roof, proj and shade layers
            gdal::create_vector_helper layers_helper ( ds_out, std::cerr );
            auto roof_layer = layers_helper.create_layer ( DEFAULT_ROOF_LAYER_NAME, wkbMultiPolygon, layer->GetSpatialRef(), force_rewrite, promt_func );
            if ( !roof_layer )
                continue;

            roof_layer << gdal::field_definition ( DEFAULT_OBJECT_ID_FIELD_NAME, OFTInteger );
            threading::shared_layer_iterator roof_layer_iter ( roof_layer );

            struct map_item
            {
                int object_index = 0;
                gdal::multipolygon roof;
            };

            int done_automatically = 0;
            int done_manually = 0;

            std::vector < map_item > map_items ( positions_list.size () );

            int key = 0;
            for ( int j = 0; j < positions_list.size (); )
            {
                auto &positions_data = positions_list [j];
                const auto &tile_tl = positions_data.first.top_left;
                auto &positions = positions_data.second;
                auto obj_id = positions_data.first.index;
                auto original_on_tile = positions_data.first.original_object * world_2_raster + ( -tile_tl );

                if ( positions.size() == 0 )
                {
                    std::cout << "Skipped feature " << obj_id << " with no positions\n";
                }
                else
                {

                    bool save = false;
                    bool draw_model = true;
                    bool show_markup = true;
                    int current_roof_index = 0;
                    //cv::namedWindow("Roof variant", cv::WINDOW_AUTOSIZE);

                    auto start = std::chrono::high_resolution_clock::now();
                    int actions_count = 0;

                    const int linear_size = static_cast < int > ( std::sqrt ( positions[0].aligned_roof->get_Area() ) );
                    const auto first_position_deviation = positions[0].deviation;
                    const auto first_position_weight = ( positions[0].value - positions[1].value ) / linear_size * raster_2_world ( 0, 0 )
                                                       / ( first_position_deviation > 1.0 ? first_position_deviation : 1.0 );


                    const bool use_manual = ( first_position_weight < min_first_pos_weight || first_position_deviation > max_first_pos_deviation )
                                            && ( a_run_mode == run_mode::on_demand )
                                            || ( a_run_mode == run_mode::manual ) ;

                    done_automatically  += use_manual ? 0 : 1;
                    done_manually       += use_manual ? 1 : 0;

                    while ( true && use_manual )
                    {

                        //std::cout << "Reveiwing " << obj_id << " with positions count " << positions.size();
                        //std::cout.flush();

                        auto tile = ( show_markup ) ? positions_data.first.tile_marked : positions_data.first.tile;

                        if ( draw_model )
                        {
                            auto current_roof = positions[current_roof_index].aligned_roof_in_tile;
                            building_renderer renderer ( tile );

                            renderer.render_position ( original_on_tile, cv::Scalar ( 0xFF, 0x00, 0x00 ) );
                            renderer.render_roof ( current_roof );
                            tile = renderer.get ();
                        }

                        cv::imshow("Roof variant", tile);
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

                            ++actions_count;
                        }
                        else if ( key == 82 || key == 84 )
                            show_markup = !show_markup;
                        else if ( key == 32 )
                            draw_model = !draw_model;
                    }

                    if (key == 87)
                        break;

                    auto finish = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> elapsed = finish - start;
                    total_time += elapsed.count();

                    if ( save || !use_manual )
                    {
                        if ( !use_manual )
                        {
                            current_roof_index = 0;
                        }

                        auto current_roof = positions[current_roof_index].aligned_roof;

                        auto & item = map_items [j];
                        auto roof_poly = instance < multipolygon > ();
                        roof_poly->addGeometryDirectly ( current_roof->clone () );
                        item.roof = roof_poly;
                        item.object_index = std::stoi ( obj_id );

                        if ( a_run_mode != run_mode::automatic )
                        {
                            ++choice_index_hist [current_roof_index];
                            choice_time << obj_id << '\t' << current_roof_index /*<< '\t' << actions_count
                                        << '\t' << elapsed.count () << "\t" << linear_size << '\t' << positions[current_roof_index].value / linear_size
                                        << "\t" << positions[ ( current_roof_index == 0 ? 1 : 0 ) ].value / linear_size
                                        << "\t" << positions[current_roof_index].deviation
                                        << "\t" << positions[( current_roof_index == 0 ? 1 : 0 )].deviation*/
                                        << "\t" << ( positions[0].value - positions[ 1 ].value ) /  linear_size * raster_2_world ( 0, 0 )
                                        << "\t" << ( positions[0].deviation )
                                        << '\n';
                            choice_time.flush ();
                        }

                        //roof_shift_stats << linear_size << "\t" << positions[current_roof_index].shift_world.norm() << '\n';
                    }

                    if ( !save && a_run_mode != run_mode::automatic )
                    {
                        auto current_roof = positions[current_roof_index].aligned_roof;
                        const int linear_size = static_cast < int > ( std::sqrt ( current_roof->get_Area() ) );

                        choice_time << obj_id << '\t' << -1 /*<< '\t' << actions_count
                                    << '\t' << elapsed.count ()<< "\t" << linear_size
                                    << '\t' << positions[0].value / linear_size
                                    << "\t" << positions[1].value / linear_size
                                    << '\t' << positions[0].deviation
                                    << "\t" << positions[1].deviation*/
                                    << '\t' << ( positions[0].value - positions[1].value ) / linear_size * raster_2_world ( 0, 0 )
                                    << '\t' << ( positions[0].deviation )
                                    << '\n';
                        choice_time.flush ();

                        ++choice_index_hist [-1];
                    }
                }

                if (key == 8)
                    --j;
                else
                    ++j;

                progress_func ( i + 1, layers, j / features_count );
            }

            choice_time << "Overall time is " << total_time << "s";

            for ( auto item : choice_index_hist )
            {
                choice_hist << item.first << '\t' << item.second << '\n';
            }

            progress_func ( i + 1, layers, 1.0f, true );

            std::cout << "Roofs aligned automatically " << done_automatically << "(" << ( done_automatically / float ( done_automatically + done_manually ) * 100.f ) << "%)"
                      << " manyally " << done_manually << "(" << ( done_manually / float ( done_automatically + done_manually ) * 100.f ) << "%)\n";

            std::cout << "Saving results...";

            gdal::shared_feature new_feature ( roof_layer_iter.layer()->GetLayerDefn() );
            for ( const auto &item : map_items )
            {
                if ( item.roof != nullptr )
                {
                    new_feature->SetGeometry ( item.roof.get () );
                    new_feature->SetField ( DEFAULT_OBJECT_ID_FIELD_NAME, item.object_index );
                    roof_layer_iter.set_feature( new_feature );
                }
            }
        }

        std::cout << "done\n";
    }
}

