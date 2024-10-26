#pragma once

#include "rsai/multiview_building_reconstructor.h"

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
#include "rsai/building_models/multiview_estimator.h"
#include "rsai/building_models/structure_estimator.h"
#include "differentiation/convolution_mask.h"
#include "differentiation/gauss_directed_derivative.h"
#include "differentiation/edge_detection.h"

using namespace rsai;

struct tiles_info
{
    std::string index;
    gdal::polygons roofs;
    std::vector < cv::Mat > tiles;
    std::vector < Eigen::Vector2d > tile_shifts;
    std::vector < Eigen::Matrix3d > raster_2_worlds;
};

template < class PromtFunc, class ProgressFunc >
rsai::multiview_building_reconstructor::multiview_building_reconstructor (
                                                                     gdal::shared_datasets &ds_vectors
                                                                   , gdal::shared_datasets &ds_rasters
                                                                   , gdal::shared_datasets &ds_bounds
                                                                   , gdal::shared_datasets &ds_outs
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
    if ( ds_vectors.size () == 0 )
        return;

    const int max_structure_length = 150;

    static const std::string id_field_name = DEFAULT_OBJECT_ID_FIELD_NAME;

    auto ds_first_vector    = ds_vectors.front();
    auto ds_first_raster    = ds_rasters.front();
    auto ds_first_bound     = ds_bounds.front();
    auto ds_first_out       = ds_outs.front();

    std::vector < std::string > structure_save_dirs;
    for ( const auto ds_out : ds_outs )
    {
        structure_save_dirs.push_back( std::string ( ds_out->GetDescription() )
                                       + "/" + DEFAULT_VARIANTS_DIRECTORY + "/" + DEFAULT_STRUCTURE_SUBDIRECTORY );
    }

    const int layers = ds_first_vector->GetLayerCount ();

    std::cout << "Reconstructing buildings...\n";

    bool layers_result = true;
    for( int i = 0; i < layers; ++i )
    {
        std::vector < threading::shared_layer_iterator > source_layer_iters;
        std::vector < threading::shared_layer_iterator > roof_layer_iters;

        std::vector < opencv::dataset_roi_extractor > ds_tile_extractors;

        for ( int ds_index = 0; ds_index < ds_vectors.size (); ++ds_index )
        {
            auto roof_layer = ds_vectors [ds_index]->GetLayer ( i );

            auto bounds_layer = ds_bounds [ds_index]->GetLayer ( i );

            source_layer_iters.emplace_back ( bounds_layer );
            roof_layer_iters.emplace_back ( roof_layer );
            ds_tile_extractors.emplace_back ( ds_rasters [ds_index] );
        }

        auto leading_roofs = ds_vectors.front();
        auto leading_bounds = ds_bounds.front();
        auto leading_raster = ds_rasters.front();
        auto &leading_layer_iter = source_layer_iters.front();
        auto leading_layer = leading_layer_iter.layer();

        rsai::building_variants_saver saver ( "" );

        std::cout << "leading_layer " << leading_layer->GetName() << " features " << leading_layer->GetFeatureCount() << '\n';

        const float features_count = leading_layer->GetFeatureCount ();
        std::atomic_int features_processed ( 0 );

        std::vector < std::pair < tiles_info, rsai::building_models::structure_responses > > structure_list;

        threading::layer_iterator a_layer_iterator ( leading_layer );
        layers_result &= a_layer_iterator ( [&] ( gdal::shared_feature feature, const int current_feature_id )
        {
            const OGRGeometry *geometry = feature->GetGeometryRef ();
            if ( geometry != nullptr
                    && wkbFlatten ( geometry->getGeometryType()) == wkbPolygon )
            {
                const int object_index = feature->GetFieldAsInteger ( id_field_name.c_str() );
                const std::string object_index_str = std::to_string ( object_index );

                //std::cout << "for ID = " << object_index_str << "\n";

                gdal::polygons roofs;
                std::vector < Eigen::Vector2d > proj_steps;
                std::vector < Eigen::Vector2d > shade_steps;
                std::vector < Eigen::Matrix3d > world_2_rasters;
                std::vector < Eigen::Matrix3d > raster_2_worlds;
                std::vector < cv::Mat > tiles;
                std::vector < Eigen::Vector2d > tile_shifts;

                {
                    eigen::gdal_dataset_bridge raster_eigen ( leading_raster );
                    Eigen::Matrix3d raster_2_world = raster_eigen.transform ();
                    raster_2_worlds.push_back ( raster_2_world );
                    Eigen::Matrix3d world_2_raster = raster_2_world.inverse ();
                    world_2_rasters.push_back ( world_2_raster );

                    Eigen::Vector2d proj_world ( feature->GetFieldAsDouble ( DEFAULT_PROJ_STEP_X_FIELD_NAME )
                                               , feature->GetFieldAsDouble ( DEFAULT_PROJ_STEP_Y_FIELD_NAME ) );

                    Eigen::Vector2d shade_world ( feature->GetFieldAsDouble ( DEFAULT_SHADE_STEP_X_FIELD_NAME )
                                                , feature->GetFieldAsDouble ( DEFAULT_SHADE_STEP_Y_FIELD_NAME ) );

                    proj_steps.push_back ( proj_world );
                    shade_steps.push_back ( shade_world );

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
                    auto tile_bbox  = ds_tile_extractors [0].raster_bbox ( tile_bounds );
                    auto tile       = ds_tile_extractors [0].roi ( tile_bbox );
                    tiles.push_back ( tile );
                    tile_shifts.push_back ( -tile_bbox.top_left() );
                }

                for ( int ds_index = 1; ds_index < ds_vectors.size (); ++ds_index )
                {
                    auto & source_layer_iter = source_layer_iters [ds_index];
                    auto features = source_layer_iter.find_feature ( id_field_name + " = " + object_index_str );

                    if ( features.size() == 1 )
                    {
                        auto ds_raster = ds_rasters [ds_index];
                        eigen::gdal_dataset_bridge raster_eigen ( ds_raster );
                        Eigen::Matrix3d raster_2_world = raster_eigen.transform ();
                        raster_2_worlds.push_back ( raster_2_world );
                        Eigen::Matrix3d world_2_raster = raster_2_world.inverse ();
                        world_2_rasters.push_back ( world_2_raster );

                        auto current_feature = features.front();
                        Eigen::Vector2d proj_world ( current_feature->GetFieldAsDouble ( DEFAULT_PROJ_STEP_X_FIELD_NAME )
                                                   , current_feature->GetFieldAsDouble ( DEFAULT_PROJ_STEP_Y_FIELD_NAME ) );

                        Eigen::Vector2d shade_world ( current_feature->GetFieldAsDouble ( DEFAULT_SHADE_STEP_X_FIELD_NAME )
                                                    , current_feature->GetFieldAsDouble ( DEFAULT_SHADE_STEP_Y_FIELD_NAME ) );

                        proj_steps.push_back ( proj_world );
                        shade_steps.push_back ( shade_world );

                        const OGRGeometry *geometry = current_feature->GetGeometryRef ();
                        if ( geometry == nullptr
                                || wkbFlatten ( geometry->getGeometryType()) != wkbPolygon )
                        {
                            std::cerr << "Objects' " << object_index_str << " geometry is not a polygon. Actual type is "
                                      << wkbFlatten ( geometry->getGeometryType()) << '\n';
                            continue;
                        }

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
                        auto tile_bbox  = ds_tile_extractors [ds_index].raster_bbox ( tile_bounds );
                        auto tile       = ds_tile_extractors [ds_index].roi ( tile_bbox );
                        tiles.push_back ( tile );
                        tile_shifts.push_back ( -tile_bbox.top_left() );
                    }
                    else
                        std::cerr << "WARNING: " << ds_index << " found " << features.size() << " features\n";
                }

                for ( int ds_index = 0; ds_index < ds_vectors.size (); ++ds_index )
                {
                    auto & roof_layer_iter = roof_layer_iters [ds_index];
                    auto roof_features = roof_layer_iter.find_feature ( id_field_name + " = " + object_index_str );

                    if ( roof_features.size() == 1 )
                    {
                        auto current_roof_feature = roof_features.front();
                        auto roof_geometry = current_roof_feature->GetGeometryRef();
                        if ( gdal::is_polygon ( roof_geometry ) )
                            roofs.push_back ( gdal::to_polygon ( roof_geometry ) );
                        else
                        {
                            roofs.push_back ( nullptr );
                            std::cerr << "WARNING: wrong geometry type for dataset " << ds_index << " actual type is " << roof_geometry->getGeometryType () << " features\n";
                        }
                    }
                    else
                        roofs.push_back ( nullptr );
                }

                const int max_length = feature->GetFieldAsDouble ( DEFAULT_VECTOR_MAX_LENGTH_NAME );

                rsai::building_models::multiview_estimator estimator ( roofs, proj_steps, shade_steps, world_2_rasters, 1, max_structure_length );

                auto structures = estimator ( tile_shifts, tiles, segmentize_step, shade_variants );

                if ( an_interaction_mode == interaction_mode::external )
                    saver.write ( object_index_str, roofs, structures, tiles, tile_shifts, raster_2_worlds, structure_save_dirs );
                else
                {
                    static std::mutex mutex;
                    std::lock_guard < std::mutex > lock ( mutex );

                    structure_list.push_back ( { { object_index_str, std::move ( roofs ), tiles, tile_shifts, std::move ( raster_2_worlds ) }, std::move ( structures ) } );
                }
            }
        }, i, layers, progress_func );

        if ( an_interaction_mode == interaction_mode::internal )
        {
            std::cout << "Result reviewing...\n";

            const std::string out_dir = ds_outs [0]->GetDescription();
            std::ofstream choice_hist ( out_dir + "struct_stats.txt" );
            std::ofstream choice_time ( out_dir + "struct_timings.txt" );
            /*choice_time << "ID" << '\t' << "index" << '\t' << "actions"
                        << '\t' << "time, s" << "\t" << "size" << "\t" << "response"
                        << "\t" << "first response" << '\n';*/

            choice_time << "index" << '\t' << "value1"
                        << '\t' << "value2" << '\t' << "error" << '\n';

            double total_time = 0.0;

            std::map < int, int > choice_index_hist;

            struct map_item
            {
                int object_index = 0;
                std::map < int, gdal::multipolygon > projes;
                std::map < int, gdal::multipolygon > shades;
            };

            std::vector < map_item > map_items ( structure_list.size () );

            int key = 0;
            for ( int j = 0; j < structure_list.size (); )
            {
                auto &structure_data = structure_list [j];
                auto obj_id = structure_data.first.index;
                const auto &roofs = structure_data.first.roofs;
                const auto &tile_shifts = structure_data.first.tile_shifts;
                const auto &tiles = structure_data.first.tiles;
                const auto &raster_2_worlds = structure_data.first.raster_2_worlds;
                auto &structures = structure_data.second;

                if ( structures.size() == 0 )
                {
                    std::cout << "Skipped feature " << obj_id << " with no structure variants\n";
                }
                else
                {
                    bool save = false;
                    bool draw_model = true;
                    int current_structure_index = 0;
                    //cv::namedWindow("Image", cv::WINDOW_AUTOSIZE);

                    auto start = std::chrono::high_resolution_clock::now();
                    int actions_count = 0;

                    while (true && a_run_mode != run_mode::automatic )
                    {
                        auto geometry_list = structures [current_structure_index].projections_and_shades;
                        for ( int i = 0; i < geometry_list.size(); ++i )
                        {
                            auto tile = tiles [i];
                            auto structure = geometry_list [i];
                            if ( !tile.empty() )
                            {
                                if ( draw_model )
                                {
                                    building_renderer renderer ( tile );

                                    if ( roofs [i] )
                                    {
                                        //std::cout << "roof " << i << " exists\n";

                                        auto roof = gdal::operator + ( structure [0], tile_shifts [i] );
                                        auto proj = gdal::operator + ( structure [1], tile_shifts [i] );
                                        auto shade = gdal::operator + ( structure [2], tile_shifts [i] );

                                        renderer.render_roof ( roof );
                                        renderer.render_projection ( proj );
                                        renderer.render_shade ( shade );

                                        auto proj_world     = gdal::operator * ( structure [1], raster_2_worlds [i] );
                                        auto shade_world    = gdal::operator * ( structure [2], raster_2_worlds [i] );

                                        tile = renderer.get ();
                                    }
//                                    else
//                                        std::cout << "roof " << i << " DOES NOT exist\n";
                                }
                            }
                            cv::imshow(std::to_string ( i ), tile);
                        }
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
                            if (key == 81 && current_structure_index > 0)
                                --current_structure_index;
                            else if (key == 83 && current_structure_index < structures.size() - 1)
                                ++current_structure_index;

                            ++actions_count;
                        }
                        else if ( key == 32 )
                            draw_model = !draw_model;
                    }

                    if (key == 87)
                        break;

                    auto finish = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> elapsed = finish - start;
                    total_time += elapsed.count();

                    if ( save || a_run_mode == run_mode::automatic )
                    {
                        int linear_size = 0;
                        for ( int i = 0; i < structures [0].projections_and_shades.size(); ++i )
                        {
                            if ( roofs [i] )
                            {
                                linear_size = static_cast < int > ( std::sqrt ( roofs [i]->get_Area() ) );
                                break;
                            }
                        }

                        if ( a_run_mode == run_mode::automatic )
                        {
                            auto response_diff = std::abs ( structures [1].response - structures [0].response ) / linear_size;
                            response_diff = response_diff < 1.0 ? 1.0 : response_diff;
                            const auto distance_diff = std::abs ( structures [1].length - structures [0].length ) / float ( max_structure_length );

                            if ( distance_diff / response_diff < 0.15 )
                            {
                                current_structure_index = 1;
                            }
                            else
                                current_structure_index = 0;
                        }

                        auto geometry_list = structures [current_structure_index].projections_and_shades;
                        for ( int i = 0; i < geometry_list.size(); ++i )
                        {
                            auto structure = geometry_list [i];

                            if ( roofs [i] )
                            {
                                auto proj_world     = gdal::operator * ( structure [1], raster_2_worlds [i] );
                                auto shade_world    = gdal::operator * ( structure [2], raster_2_worlds [i] );

                                auto & item = map_items [j];
                                item.object_index = std::stoi ( obj_id );
                                item.projes [i] = proj_world;
                                item.shades [i] = shade_world;


                            }
                        }

                        if ( a_run_mode != run_mode::automatic )
                        {
                            const auto response_diff = std::abs ( structures [1].response - structures [0].response ) / linear_size;
                            const auto distance_diff = std::abs ( structures [1].length - structures [0].length ) / float ( max_structure_length );

                            ++choice_index_hist [current_structure_index];
                            choice_time /*<< obj_id << '\t'*/ << current_structure_index /*<< '\t' << actions_count
                                        << '\t' << elapsed.count () << "\t" << linear_size*/
                                        << "\t" << ( response_diff < 1.0 ? 1.0 : response_diff )
                                        << "\t" << distance_diff;
                            /*if ( current_structure_index > 0 )
                                choice_time << "\t" << ( structures [current_structure_index].shade_weight - structures [current_structure_index + 1].shade_weight )
                                            << "\t" << ( structures [current_structure_index].shade_weight - structures [current_structure_index - 1].shade_weight );*/
                            choice_time << '\n';
                            choice_time.flush ();
                        }
                    }

                    if ( !save && a_run_mode != run_mode::automatic )
                    {
                        auto current_roof = roofs[0];
                        const int linear_size = ( current_roof ) ? static_cast < int > ( std::sqrt ( current_roof->get_Area() ) ) : 1.0;

//                        choice_time /*<< obj_id << '\t'*/ << -1 /*<< '\t' << actions_count
//                                    << '\t' << elapsed.count () << "\t" << linear_size*/
//                                    << "\t" << std::abs ( structures [1].response / linear_size - structures [0].response / linear_size )
//                                    << "\t" << structures [1].shade_weight
//                                    << '\n';
//                        choice_time.flush ();

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

            std::vector < threading::shared_layer_iterator > proj_layer_iters;
            std::vector < threading::shared_layer_iterator > shade_layer_iters;

            for ( int ds_index = 0; ds_index < ds_vectors.size (); ++ds_index )
            {
                auto bounds_layer = ds_bounds [ds_index]->GetLayer ( i );

                // Creating output roof, proj and shade layers
                gdal::create_vector_helper layers_helper ( ds_outs [ds_index], std::cerr );
                auto proj_layer = layers_helper.create_layer ( DEFAULT_PROJECTION_LAYER_NAME, wkbMultiPolygon, bounds_layer->GetSpatialRef(), force_rewrite, promt_func );
                if ( !proj_layer )
                    return;

                proj_layer << gdal::field_definition ( DEFAULT_OBJECT_ID_FIELD_NAME, OFTInteger );

                auto shade_layer = layers_helper.create_layer ( DEFAULT_SHADE_LAYER_NAME, wkbMultiPolygon, bounds_layer->GetSpatialRef(), force_rewrite, promt_func );
                if ( !shade_layer )
                    return;

                shade_layer << gdal::field_definition ( DEFAULT_OBJECT_ID_FIELD_NAME, OFTInteger );

                proj_layer_iters.emplace_back ( proj_layer );
                shade_layer_iters.emplace_back ( shade_layer );
            }

            std::cout << "Saving results...";
            for ( const auto &item : map_items )
            {
                for ( auto proj_pair : item.projes )
                {
                    const int layer_index = proj_pair.first;
                    const auto &shade_iter = item.shades.find ( layer_index );
                    if ( shade_iter == item.shades.end () )
                    {
                        std::cerr << "Warning: shade for object " << item.object_index << "is not found\n";
                        continue;
                    }

                    gdal::shared_feature proj_feature ( proj_layer_iters [layer_index].layer()->GetLayerDefn() );
                    proj_feature->SetGeometry ( proj_pair.second.get () );
                    proj_feature->SetField ( DEFAULT_OBJECT_ID_FIELD_NAME, item.object_index );
                    proj_layer_iters [layer_index].set_feature( proj_feature );

                    gdal::shared_feature shade_feature ( shade_layer_iters [layer_index].layer()->GetLayerDefn() );
                    shade_feature->SetGeometry ( shade_iter->second.get () );
                    shade_feature->SetField ( DEFAULT_OBJECT_ID_FIELD_NAME, item.object_index );
                    shade_layer_iters [layer_index].set_feature( shade_feature );
                }
            }

            std::cout << "done\n";
        }
    }
}

