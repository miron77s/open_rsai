#include <iostream>
#include <string>

#include <args-parser/all.hpp>

#include "rsai/projection_and_shade_locator.h"

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
        input_vector_param.setDescription( input_vector_param.description() + SL( "Used to generate buildings shades and projections." ) );

        Args::Arg & input_raster_param = arguments::get_input_raster ();
        input_raster_param.setDescription( input_raster_param.description() + SL ( "Used to acquire raster-to-world resolution. "
                                           "Raster is used to find actual buildings' shades and projection boundaries." ) );

        Args::Arg & output_param = arguments::get_output ();
        output_param.setDescription( output_param.description() + "It is saved into the maps named "
                                     "'" + DEFAULT_ROOF_LAYER_NAME + "', "
                                     "'" + DEFAULT_PROJECTION_LAYER_NAME + "' and "
                                     "'" + DEFAULT_SHADE_LAYER_NAME + "'.");

        Args::Arg & driver_param = arguments::get_driver ();

        Args::Arg & force_rewtire_param = arguments::get_force_rewtire ();

        Args::Arg & segmentize_step_param = arguments::get_segmentize_step ();

        Args::Arg & projection_step_param = arguments::get_projection_step ();

        Args::Arg & run_mode_param = arguments::get_run_mode();

        Args::Arg & interaction_mode_param = arguments::get_interaction_mode();

        Args::Arg & roof_position_walk_param = arguments::get_roof_position_walk ();

        Args::Arg & roof_variants_param = arguments::get_roof_varians();

        Args::Arg & shade_variants_param = arguments::get_shade_varians();

        Args::Arg & use_sam_param = arguments::get_use_sam ();

        Args::Help help;
        help.setAppDescription(
            SL( "Utility to reconstruct roof-projection-shade buildings' structure from images. Each object is saved into roofs, projes and shades datasets. "
                "The reconstruction can be performed in automatic mode with no user interaction, manual mode with total results revision "
                "and 'on demand' mode to solve complex issues. "
                "World coordinate systems for all input datasets should match otherwise the utility will fail (no reprojection or conversion is performed)." ) );
        help.setExecutable( argv[0] );

        cmd.addArg ( input_vector_param );
        cmd.addArg ( input_raster_param );
        cmd.addArg ( output_param );
        cmd.addArg ( driver_param );
        cmd.addArg ( segmentize_step_param );
        cmd.addArg ( projection_step_param );
        cmd.addArg ( force_rewtire_param );
        cmd.addArg ( run_mode_param );
        cmd.addArg ( interaction_mode_param );
        cmd.addArg ( roof_position_walk_param );
        cmd.addArg ( roof_variants_param );
        cmd.addArg ( shade_variants_param );
        cmd.addArg ( use_sam_param );
        cmd.addArg ( help );

        cmd.parse();

        // loading and validating datasets
        gdal::open_vector_ro_helper iv_helper ( input_vector_param.value(), std::cerr );
        auto ds_vector = iv_helper.validate ( true );

        gdal::open_raster_ro_helper ir_helper ( input_raster_param.value(), std::cerr );
        auto ds_raster = ir_helper.validate ( true );

        gdal::create_vector_helper out_helper ( output_param.value(), driver_param.value(), std::cerr );
        auto ds_out = out_helper.validate ( true );

        if ( ds_vector == nullptr || ds_raster == nullptr || ds_out == nullptr )
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

        // Building model segmentize step value
        value_helper < double > segmentize_step_helper      ( segmentize_step_param.value() );
        if ( !segmentize_step_helper.verify ( std::cerr,    "STOP: Building model segmentize step value is incorrect." ) )
            return 1;

        // Building model projection step value
        value_helper < int > projection_step_helper         ( projection_step_param.value() );
        if ( !projection_step_helper.verify ( std::cerr,    "STOP: Building model projection step value are incorrect." ) )
            return 1;

        // Building model roof position walk value
        value_helper < double > roof_position_walk_helper   ( roof_position_walk_param.value() );
        if ( !roof_position_walk_helper.verify ( std::cerr, "STOP: Building model roof position walk value is incorrect." ) )
            return 1;

        // Building model roof varinats to generate
        value_helper < int > roof_variants_helper           ( roof_variants_param.value() );
        if ( !roof_variants_helper.verify ( std::cerr,      "STOP: Building model roof variants value is incorrect." ) )
            return 1;

        value_helper < int > shade_variants_helper          ( shade_variants_param.value() );
        if ( !shade_variants_helper.verify ( std::cerr,      "STOP: Building model roof variants value is incorrect." ) )
            return 1;

        const double segmentize_step = segmentize_step_helper.value();
        const int projection_step = projection_step_helper.value();
        const double roof_position_walk = roof_position_walk_helper.value();
        const int roof_variants = roof_variants_helper.value();
        const int shade_variants = shade_variants_helper.value();

        const auto run_mode = rsai::run_mode_from_string ( run_mode_param.value() );
        const auto interaction_mode = rsai::interaction_mode_from_string ( interaction_mode_param.value() );

        if ( run_mode == run_mode::invalid )
        {
            std::cerr << "STOP: Utility run mode '" << run_mode_param.value() << "' is invalid. Use --help param for correct values." << std::endl;
            return 1;
        }

        if ( interaction_mode == interaction_mode::invalid )
        {
            std::cerr << "STOP: Utility user interaction mode '" << interaction_mode_param.value() << "' is invalid. Use --help param for correct values." << std::endl;
            return 1;
        }

        rsai::projection_and_shade_locator finder (
                                                ds_vector
                                              , ds_raster
                                              , ds_out
                                              , segmentize_step
                                              , projection_step
                                              , roof_position_walk
                                              , roof_variants
                                              , shade_variants
                                              , force_rewtire_param.isDefined ()
                                              , use_sam_param.isDefined()
                                              , run_mode
                                              , interaction_mode
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
