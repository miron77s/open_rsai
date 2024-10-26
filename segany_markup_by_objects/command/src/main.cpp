#include <iostream>
#include <string>

#include <args-parser/all.hpp>

#include "common/definitions.h"
#include "common/arguments.h"
#include "common/promt_functions.hpp"
#include "common/progress_functions.hpp"
#include "gdal_utils/all_helpers.h"
#include "eigen_utils/vector_helpers.h"
#include "rsai/segany_markup_by_objects.h"

using namespace std;

int main ( int argc, char * argv[] )
{
    try
    {
        Args::CmdLine cmd( argc, argv );

        Args::Arg &input_vector_param = arguments::get_input_vector ();
        input_vector_param.setDescription( input_vector_param.description() + SL( "Has to be raster-bounded and is used to generate Segment Anything markup." ) );

        Args::Arg & input_raster_param = arguments::get_input_raster ();
        input_raster_param.setDescription( input_raster_param.description() + SL ( "Used to create per-object tiles for further segmentation. ") );

        Args::Arg & output_param = arguments::get_output ();
        output_param.setDescription( output_param.description() + "Each object is represented by it's images tile (jpg), tile raster bounding box (bbox), "
                                                                  "tile WKT raster etalon polygon to calculate IoU (object), and tile's x,y offset (shift). "
                                                                  "Segment Anything adds a tile-rendered best IoU-matching prediction (pred.jpg) and "
                                                                  " the IoU value itself (iou).");

        Args::Arg & output_map_param = arguments::get_output_map ();
        output_map_param.setDescription( output_param.description() + "Used to store a vector markup for debug and visualization. "
                                                                      "Saved with " + DEFAULT_VECTOR_MAP_OUTPUT_DRIVER + "." );

        Args::Arg &tile_buffer_size_param = arguments::get_tile_buffer_size ();

        Args::Arg & force_rewtire_param = arguments::get_force_rewtire ();

        Args::Help help;
        help.setAppDescription(
            SL( "Utility to create a Segment Anything markup using roofs vector layer and start a script to collect best IoU-based predicitions."
                "The Segment Anything should be install to the system to run. Refer https://github.com/facebookresearch/segment-anything for details.") );
        help.setExecutable( argv[0] );

        cmd.addArg ( input_vector_param );
        cmd.addArg ( input_raster_param );
        cmd.addArg ( output_param );
        cmd.addArg ( output_map_param );
        cmd.addArg ( tile_buffer_size_param );
        cmd.addArg ( force_rewtire_param );
        cmd.addArg ( help );

        cmd.parse();

        // loading and validating datasets
        gdal::open_vector_ro_helper iv_helper ( input_vector_param.value(), std::cerr );
        auto ds_vector = iv_helper.validate ( true );

        gdal::open_raster_ro_helper ir_helper ( input_raster_param.value(), std::cerr );
        auto ds_raster = ir_helper.validate ( true );

        gdal::shared_dataset ds_out;

        if ( output_map_param.isDefined () )
        {
            gdal::create_vector_helper out_helper ( output_map_param.value(), DEFAULT_VECTOR_MAP_OUTPUT_DRIVER, std::cerr );
            ds_out = out_helper.validate ( true );
        }

        if ( ds_vector == nullptr || ds_raster == nullptr )
        {
            std::cerr << "STOP: Input parameters verification failed" << std::endl;
            return 1;
        }

        // getting and validaring SRSs
        auto vec_srs = iv_helper.srs(),
             rst_srs = ir_helper.srs();

        // Verifying SRSs
        if ( !vec_srs.verify ( std::cerr ) || !rst_srs.verify ( std::cerr ) )
        {
            std::cerr << "STOP: Input datasets' SRS are not valid" << std::endl;
            return 1;
        }

        // Verifying SRSs similarity
        if ( !vec_srs->IsSame ( rst_srs.get() ) )
        {
            std::cerr << "STOP: source vector map's and raster image's SRSs are not same." << std::endl << std::endl;
            std::cout << "Vector SRS: " << vec_srs.export_to_pretty_wkt() << std::endl << std::endl;
            std::cout << "Raster SRS: " << rst_srs.export_to_pretty_wkt() << std::endl;
            return 1;
        }

        value_helper < int > tile_buffer_size_helper  ( tile_buffer_size_param.value() );

        if ( !tile_buffer_size_helper.verify ( std::cerr, "STOP: Geometry buffer tile size is incorrect." ) )
            return 1;

        const int tile_buffer_size = tile_buffer_size_helper.value();

        rsai::segany_markup_by_objects markupper (
                                                      ds_vector
                                                    , ds_raster
                                                    , ds_out
                                                    , output_param.value ()
                                                    , tile_buffer_size
                                                    , force_rewtire_param.isDefined ()
                                                    , rewrite_directory_promt_func
                                                    , rewrite_layer_promt_func
                                                    , console_progress_layers
                                                 );
    }
    catch( const Args::HelpHasBeenPrintedException & )
    {
    }
    catch( const Args::BaseException & x )
    {
        Args::outStream() << x.desc() << SL( "\n" );
    }

    return 0;
}
