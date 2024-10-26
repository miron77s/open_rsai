#include <iostream>
#include <string>

#include <args-parser/all.hpp>

#include "common/definitions.h"
#include "common/promt_functions.hpp"
#include "common/arguments.h"
#include "rsai/raster_inliers_extractor.h"
#include "common/string_utils.h"

using namespace std;

int main ( int argc, char * argv[] )
{
    try
    {
        Args::CmdLine cmd( argc, argv );

        Args::Arg &input_vector_param = arguments::get_input_vector ();
        input_vector_param.setDescription( input_vector_param.description() + SL( "Used to find raster inlier objects." ) );

        Args::Arg & input_raster_param = arguments::get_input_raster ();
        input_raster_param.setDescription( input_raster_param.description() + SL ( "Used to acquire raster-to-world resolution. "
                                           "Raster bounds are used as the primary region of interest to extract inliers." ) );

        Args::Arg & output_param = arguments::get_output ();
        output_param.setDescription( output_param.description() + "It is saved into the map named '" + DEFAULT_INLIERS_LAYER_NAME + "'." );

        Args::Arg & driver_param = arguments::get_driver ();

        Args::Arg & force_rewtire_param = arguments::get_force_rewtire ();

        Args::Arg & roi_param = arguments::get_region_of_interest ();
        roi_param.setDescription( roi_param.description () + SL( "If is set it's intersection with raster bound designates the final region to extract inliers. "
                                                                 "If is not set - the roi is a whole input raster.") );

        Args::Arg & crop_geometry_param = arguments::get_crop_by_raster ();

        Args::Arg & semantic_filter_param = arguments::get_semantic_filter ();

        Args::Arg & min_raster_obj_size_param = arguments::get_min_raster_obj_size ();

        Args::Help help;
        help.setAppDescription(
            std::string ( "Utility to extract vector objects sited inside the raster image bounds. "
                "The objects' geometry can be cropped by it if '" ) + crop_geometry_param.name() + "' parameter is set. "
                "Also semantic object filtering is available with '" + semantic_filter_param.name () + "' param. "
                "World coordinate systems for all input datasets should match otherwise the utility will fail (no reprojection or conversion is performed)." );
        help.setExecutable( argv[0] );

        cmd.addArg ( input_vector_param );
        cmd.addArg ( input_raster_param );
        cmd.addArg ( output_param );
        cmd.addArg ( driver_param );
        cmd.addArg ( roi_param );
        cmd.addArg ( force_rewtire_param );
        cmd.addArg ( crop_geometry_param );
        cmd.addArg ( semantic_filter_param );
        cmd.addArg ( min_raster_obj_size_param );
        cmd.addArg ( help );

        cmd.parse();

        // loading and validating datasets
        gdal::open_vector_ro_helper iv_helper ( input_vector_param.value(), std::cerr );
        auto ds_vector = iv_helper.validate ( true );

        gdal::open_raster_ro_helper ir_helper ( input_raster_param.value(), std::cerr );
        auto ds_raster = ir_helper.validate ( true );

        gdal::create_vector_helper out_helper ( output_param.value(), driver_param.value(), std::cerr );
        auto ds_out = out_helper.validate ( true );

        gdal::open_vector_ro_helper roi_helper ( roi_param.value(), std::cerr );
        auto ds_roi = roi_helper.validate ( false );

        if ( ds_vector == nullptr || ds_raster == nullptr || ds_out == nullptr || ds_roi == nullptr && !roi_param.value().empty () )
        {
            std::cerr << "STOP: Input parameters verification failed" << std::endl;
            return 1;
        }

        // getting and validaring SRSs
        auto vec_srs = iv_helper.srs(),
             rst_srs = ir_helper.srs(),
             roi_srs = roi_helper.srs ();

        // Verifying SRSs
        if ( !vec_srs.verify ( std::cerr ) || !rst_srs.verify ( std::cerr ) )
        {
            std::cerr << "STOP: Input datasets' SRS are not valid" << std::endl;
            return 1;
        }

        if ( ( ds_roi != nullptr ) && !roi_srs.verify ( std::cerr ) )
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

        if ( ( ds_roi != nullptr ) && !vec_srs->IsSame ( roi_srs.get() ) )
        {
            std::cerr << "STOP: source vector map's and region of interest SRSs are not same." << std::endl << std::endl;
            std::cout << "Vector SRS: " << vec_srs.export_to_pretty_wkt() << std::endl << std::endl;
            std::cout << "ROI SRS: " << roi_srs.export_to_pretty_wkt() << std::endl;
            return 1;
        }

        // Get max vectors length and mask size
        value_helper < int > raster_size_helper    ( min_raster_obj_size_param.value() );
        if ( !raster_size_helper.verify ( std::cerr, "STOP: Minimal object size on raster is incorrect" ) )
        {
            return 1;
        }

        rsai::raster_inliers_extractor extractor (
                                                    ds_vector
                                                  , ds_raster
                                                  , ds_out
                                                  , ds_roi
                                                  , force_rewtire_param.isDefined ()
                                                  , crop_geometry_param.isDefined ()
                                                  , semantic_filter_param.value()
                                                  , raster_size_helper.value()
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
