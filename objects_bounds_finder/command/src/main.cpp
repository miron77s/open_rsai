#include <iostream>
#include <string>

#include <args-parser/all.hpp>

#include "rsai/objects_bounds_finder.h"

#include "common/definitions.h"
#include "common/arguments.h"
#include "common/promt_functions.hpp"
#include "common/progress_functions.hpp"

using namespace std;

int main ( int argc, char * argv[] )
{
    try
    {
        Args::CmdLine cmd( argc, argv );

        Args::Arg &input_vector_param = arguments::get_input_vector ();
        input_vector_param.setDescription( input_vector_param.description() + SL( "Used to generate objectwise raster boundaries." ) );

        Args::Arg & input_raster_param = arguments::get_input_raster ();
        input_raster_param.setDescription( input_raster_param.description() + SL ( "Used to acquire raster-to-world resolution. "
                                         "Raster bounding box is used to ensure the bounds are inside." ) );

        Args::Arg &input_metadata_param = arguments::get_input_metadata();
        input_metadata_param.setDescription( input_metadata_param.description() + SL( "Contains projection and shading vectors" ) );

        Args::Arg & output_param = arguments::get_output ();
        output_param.setDescription( output_param.description() + "It is saved into the map named '" + DEFAULT_BOUNDS_LAYER_NAME + "'."  );

        Args::Arg & driver_param = arguments::get_driver ();

        Args::Arg & force_rewtire_param = arguments::get_force_rewtire ();

        Args::Arg & max_proj_param = arguments::get_max_projection_length ();

        Args::Arg & mask_size_param = arguments::get_mask_size ();

        Args::Help help;
        help.setAppDescription(
            SL( "Utility to create objectswise bounding boxes accounting projecting, shading and border extactor FIR filter mask size. "
                "Bbox is saved as an exterior ring in polygon's geometry, projecting and shading steps alone with maximum length - in feature's semantics. "
                "Also performs a polygons simplification - removes source polygons interior rings. "
                "World coordinate systems for all input datasets should match otherwise the utility will fail (no reprojection or conversion is performed)." ) );
        help.setExecutable( argv[0] );

        cmd.addArg ( input_vector_param );
        cmd.addArg ( input_raster_param );
        cmd.addArg ( input_metadata_param );
        cmd.addArg ( output_param );
        cmd.addArg ( driver_param );
        cmd.addArg ( mask_size_param );
        cmd.addArg ( max_proj_param );
        cmd.addArg ( force_rewtire_param );
        cmd.addArg ( help );

        cmd.parse();

        // loading and validating datasets
        gdal::open_vector_ro_helper iv_helper ( input_vector_param.value(), std::cerr );
        auto ds_vector = iv_helper.validate ( true );

        gdal::open_raster_ro_helper ir_helper ( input_raster_param.value(), std::cerr );
        auto ds_raster = ir_helper.validate ( true );

        gdal::open_vector_ro_helper im_helper ( input_metadata_param.value(), std::cerr );
        auto ds_meta = im_helper.validate ( true );

        gdal::create_vector_helper out_helper ( output_param.value(), driver_param.value(), std::cerr );
        auto ds_out = out_helper.validate ( true );

        if ( ds_vector == nullptr || ds_raster == nullptr || ds_meta == nullptr || ds_out == nullptr )
        {
            std::cerr << "STOP: Input parameters verification failed" << std::endl;
            return 1;
        }

        // getting and validaring SRSs
        auto vec_srs = iv_helper.srs(),
             rst_srs = ir_helper.srs(),
             meta_srs = im_helper.srs();

        // Verifying SRSs
        if ( !vec_srs.verify ( std::cerr ) || !rst_srs.verify ( std::cerr ) || !meta_srs.verify ( std::cerr ) )
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

        if ( !vec_srs->IsSame ( meta_srs.get() ) )
        {
            std::cerr << "STOP: source vector map's and projectnios metadata SRSs are not same." << std::endl << std::endl;
            std::cout << "Vector SRS: " << vec_srs.export_to_pretty_wkt() << std::endl << std::endl;
            std::cout << "Metadata SRS: " << meta_srs.export_to_pretty_wkt() << std::endl;
            return 1;
        }

        // Get max vectors length and mask size
        value_helper < int > length_helper  ( max_proj_param.value() );
        value_helper < int > mask_helper    ( mask_size_param.value() );

        if ( !length_helper.verify ( std::cerr, "STOP: Maximum vectors length is incorrect" )
             || !mask_helper.verify ( std::cerr, "STOP: Maximum mask size is incorrect" ) )
        {
            return 1;
        }

        const int max_project_length = length_helper.value();
        const int mask_size = mask_helper.value();

        rsai::objects_bounds_finder finder (
                                                ds_vector
                                              , ds_raster
                                              , ds_meta
                                              , ds_out
                                              , mask_size
                                              , max_project_length
                                              , force_rewtire_param.isDefined ()
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
