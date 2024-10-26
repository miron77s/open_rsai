#include "rsai/building_variants_saver.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "rsai/building_models/building_renderer.h"

using namespace rsai;

const std::string source_tile_name = "src";
const std::string roof_wkt_file_ext = ".roof";
const std::string proj_wkt_file_ext = ".proj";
const std::string shade_wkt_file_ext = ".shade";
const std::string image_file_ext = ".jpg";

rsai::building_variants_saver::building_variants_saver ( const std::string &target_dir )
    : m_target_dir ( target_dir )
{
    if (!std::filesystem::exists(target_dir) && !target_dir.empty())
        std::filesystem::create_directories(target_dir);
}

bool rsai::building_variants_saver::write ( const std::string id, const building_models::structures &positions, const cv::Mat &tile
                                           , const Eigen::Vector2d & tile_top_left, const Eigen::Matrix3d &raster_2_world )
{
    const std::string sample_target = m_target_dir + "/" + id + "/";

    if (!std::filesystem::exists(sample_target))
        std::filesystem::create_directories(sample_target);

    // Saving source tile without vectors
    cv::imwrite ( sample_target + source_tile_name + image_file_ext, tile );

    for ( int position_index = 0; position_index < positions.size (); ++position_index )
    {
        const auto & position = positions [position_index];

        const std::string sample_position_target = sample_target + std::to_string ( position_index ) + "/";
        if (!std::filesystem::exists(sample_position_target))
            std::filesystem::create_directories(sample_position_target);

        // Save the roof image for the first choice
        if ( position.shades.size() > 0 )
        {
            const auto roof = position.shades [0].model.roof();

            building_renderer roof_renderer ( tile );
            roof_renderer.render_roof ( roof );

            const auto roof_image_name = sample_target + std::to_string ( position_index ) + image_file_ext;
            roof_renderer.save ( roof_image_name );
        }

        // Save roofs, projes and shades to choose final model
        for ( int shade_index = 0; shade_index < position.shades.size (); ++shade_index )
        {
            const std::string projection_position_target = sample_position_target + std::to_string ( shade_index );

            const auto &variant = position.shades [shade_index];

            auto model = variant.model;

            building_renderer renderer ( tile );

            renderer.render_roof ( model.roof() );
            renderer.render_projection ( model.projection() );
            renderer.render_shade ( model.shade() );

            renderer.save ( projection_position_target + image_file_ext );

            model.transform_2_world ( raster_2_world, tile_top_left );
            auto roof = model.roof();
            auto proj = model.projection();
            auto shade = model.shade();

            __write_wkt ( projection_position_target + roof_wkt_file_ext, roof.get() );
            __write_wkt ( projection_position_target + proj_wkt_file_ext, proj.get() );
            __write_wkt ( projection_position_target + shade_wkt_file_ext, shade.get() );
        }
    }

    return true;
}

bool rsai::building_variants_saver::write ( const std::string id, gdal::polygon roof_geometry, const building_models::roof_responses &positions, const cv::Mat &tile
                                            , const Eigen::Vector2d & tile_top_left, const Eigen::Matrix3d &raster_2_world )
{
    const std::string sample_target = m_target_dir + "/" + id + "/";

    if (!std::filesystem::exists(sample_target))
        std::filesystem::create_directories(sample_target);

    // Saving source tile without vectors
    cv::imwrite ( sample_target + source_tile_name + image_file_ext, tile );

    for ( int position_index = 0; position_index < positions.size (); ++position_index )
    {
        const auto & position = positions [position_index];

        building_renderer roof_renderer ( tile );
        roof_renderer.render_roof ( position.aligned_roof_in_tile );

        const auto roof_image_name = sample_target + std::to_string ( position_index ) + image_file_ext;
        roof_renderer.save ( roof_image_name );

        __write_wkt ( sample_target + std::to_string ( position_index ) + roof_wkt_file_ext, position.aligned_roof.get() );
    }

    return true;
}

bool rsai::building_variants_saver::write ( const std::string id, std::vector < gdal::polygon > roofs, const building_models::structure_responses &structures
                                            , const std::vector < cv::Mat > &tiles, const std::vector < Eigen::Vector2d > tile_shifts
                                            , const std::vector < Eigen::Matrix3d > &raster_2_worlds, const std::vector < std::string > &save_dirs  )
{
    std::vector < std::string > sample_targets;
    for ( const auto save_dir : save_dirs )
    {
        const std::string sample_target = save_dir + "/" + id + "/";
        sample_targets.push_back ( sample_target );

        if (!std::filesystem::exists(sample_target))
            std::filesystem::create_directories(sample_target);
    }

    if ( structures.size () > 0 )
    {
        for ( int l = 0; l < structures.size (); ++l )
        {
            auto geometry_list = structures [l].projections_and_shades;
            //auto geometry_list = structures.front ().projections_and_shades;
            for ( int i = 0; i < geometry_list.size(); ++i )
            {
                auto tile = tiles [i];
                auto structure = geometry_list [i];
                if ( !tile.empty() )
                {
                    building_renderer renderer ( tile );

                    if ( roofs [i] )
                    {
                        auto roof = gdal::operator + ( structure [0], tile_shifts [i] );
                        auto proj = gdal::operator + ( structure [1], tile_shifts [i] );
                        auto shade = gdal::operator + ( structure [2], tile_shifts [i] );

                        renderer.render_roof ( roof );
                        renderer.render_projection ( proj );
                        renderer.render_shade ( shade );

                        auto proj_world     = gdal::operator * ( structure [1], raster_2_worlds [i] );
                        auto shade_world    = gdal::operator * ( structure [2], raster_2_worlds [i] );
                        __write_wkt ( sample_targets [i] + std::to_string ( l ) + proj_wkt_file_ext, proj_world.get() );
                        __write_wkt ( sample_targets [i] + std::to_string ( l ) + shade_wkt_file_ext, shade_world.get() );
                    }

                    const auto roof_image_name = sample_targets [i] + std::to_string ( l ) + image_file_ext;
                    renderer.save ( roof_image_name );
                }
            }
        }
    }

    return true;
}

bool rsai::building_variants_saver::__write_wkt ( const std::string &file_name, const OGRGeometry * geom ) const
{
    auto wkt = gdal::to_wkt ( geom );

    std::ofstream file ( file_name );
    if ( file )
    {
        file << wkt;
        file.close();
        return true;
    }
    else
        return false;
}
