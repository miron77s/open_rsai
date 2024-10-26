#pragma once

#include "rsai/markup_writer.h"

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
#include "rsai/markup/writers.h"

using namespace rsai;

struct raster_markup
{
    gdal::shared_dataset raster;
    std::string group;
    std::string name;
    gdal::shared_datasets vectors;
};

using raster_markups = std::list < raster_markup >;

template < class PromtFunc, class ProgressFunc >
rsai::markup_writer::markup_writer (
                                                                     const strings &vector_maps
                                                                   , gdal::shared_datasets &ds_rasters
                                                                   , gdal::shared_dataset ds_denied
                                                                   , const std::string &output_dir
                                                                   , const bool force_rewrite
                                                                   , const double balance
                                                                   , const double overlap
                                                                   , const double validation
                                                                   , const Eigen::Vector2i tile_sizes
                                                                   , const strings &classes
                                                                   , const rsai::markup::markup_type &format
                                                                   , const write_mode &mode
                                                                   , const PromtFunc &promt_func
                                                                   , const ProgressFunc &progress_func
                                                                  )
{
    if ( ds_rasters.size () == 0 || vector_maps.size () == 0 )
        return;

    std::cout << "Matching rasters and vector maps...\n";

    // Matching rasters with vector maps' markup
    raster_markups markups;
    for ( auto ds_raster : ds_rasters )
    {
        const auto raster_path = ds_raster->GetDescription();
        std::filesystem::path path (raster_path);
        const auto raster_file_name = path.stem().string();
        const auto group_name = path.parent_path().filename().string();

        raster_markup markup { ds_raster, group_name, raster_file_name, {} };

        std::cout << "For raster " << raster_file_name << ", group " << group_name << '\n';

        for ( const auto &vector_name : vector_maps )
        {
            const auto vector_path = vector_name + "/" + raster_file_name;
            if ( vector_path.find ( group_name ) != std::string::npos )
            {
                gdal::open_vector_ro_helper iv_helper ( vector_path, std::cerr );
                auto ds_vector = iv_helper.validate ( false );

                if ( ds_vector != nullptr )
                {
                    markup.vectors.push_back ( ds_vector );
                    std::cout << "\tfound map " << ds_vector->GetDescription() << '\n';
                }
            }
        }

        markups.push_back ( std::move ( markup ) );
    }

    std::cout << "Generating and populating tiles...\n";

    // Creating and populating tiles
    rsai::markup::tiles tiles;
    for ( auto & markup : markups )
    {
        rsai::markup::tile_generator generator ( markup.raster, markup.group + "_" + markup.name, ds_denied );
        auto new_tiles = generator ( tile_sizes, overlap );

        std::cout << "\tRaster " << markup.group << "/" << markup.name << '\n';

        for ( int i = 0; i < markup.vectors.size(); ++i )
        {
            auto &vector = markup.vectors [i];

            float tile_index = 1;
            for ( auto &tile : new_tiles )
            {
                tile.populate ( vector, classes );
                progress_func ( i + 1, markup.vectors.size(), ( tile_index++ ) / new_tiles.size () );
            }
        }

        progress_func ( markup.vectors.size(), markup.vectors.size(), 1.0f, true );
        std::cout << "\tGenerated " << new_tiles.size () << " tiles of size " << tile_sizes.transpose() << "\n";

        tiles.splice ( tiles.end (), new_tiles );
    }

    std::cout << "Balancing tiles...\n";
    rsai::markup::tile_balancer balancer ( tiles );
    tiles = balancer ( balance );

    int populated = 0, not_populated = 0;
    int population = 0;
    for ( auto &tile : tiles )
    {
        if ( tile.populated () )
        {
            ++populated;
            population += tile.population_size();
        }
        else
            ++not_populated;
    }

    std::cout << "\tMarked tiles " << populated << "/" << float (populated) / tiles.size() * 100.f << "% "
              << "clean tiles " << not_populated << "/" << float (not_populated) / tiles.size() * 100.f << "% "
              << "with objects count " << population << '\n';

    std::cout << "Splitting train/validation tiles...\n";
    rsai::markup::tile_splitter splitter ( tiles, validation );
    auto train = splitter.train();
    auto valid = splitter.validation();

    int train_population = 0;
    for ( auto &tile : train )
        train_population += tile.population_size();

    int valid_population = 0;
    for ( auto &tile : valid )
        valid_population += tile.population_size();

    std::cout << "\tTrain part " << train.size () << "/" << float (train.size ()) / tiles.size() * 100.f << "% with " << train_population << " objects, "
              << "validation part " << valid.size () << "/" << float (valid.size ()) / tiles.size() * 100.f << "% with " << valid_population << " objects" << '\n';

    auto writer = rsai::markup::writer::get_writer ( format, classes );

    std::cout << "Saving training data...\n";
    writer->save ( train, output_dir, rsai::markup::markup_part::train, mode );

    std::cout << "Saving validation data...\n";
    writer->save ( valid, output_dir, rsai::markup::markup_part::valid, write_mode::append );
}

