#include <iostream>
#include <string>

#include <args-parser/all.hpp>

#include "rsai/multiview_building_reconstructor.h"

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
        input_vector_param.setDescription( SL( "A comma-separated list of input vector maps' file names. Each map stores roof geometries. "
                                               "Used to generate buildings shades and projections." ) );

        Args::Arg & input_raster_param = arguments::get_input_raster ();
        input_raster_param.setDescription( SL ( "A comma-separated list of input rasters' file names. Used to acquire raster-to-world resolution. "
                                           "Rasters are used to find actual buildings' shades and projection boundaries." ) );

        Args::Arg &input_bounds_param = arguments::get_input_bounds ();
        input_bounds_param.setDescription( SL( "A comma-separated list of input vector maps' file names. Each map stores bouldings' bounds on raster and shape geometries. "
                                               "Used to generate buildings shades and projections." ) );

        Args::Arg & output_param = arguments::get_output ();
        output_param.setDescription( std::string ( "A comma-separated list of output directories to store the results. in each directory the result is saved into the maps named " ) +
                                     "'" + DEFAULT_PROJECTION_LAYER_NAME + "' and "
                                     "'" + DEFAULT_SHADE_LAYER_NAME + "'.");

        Args::Arg & driver_param = arguments::get_driver ();

        Args::Arg & force_rewtire_param = arguments::get_force_rewtire ();

        Args::Arg & segmentize_step_param = arguments::get_segmentize_step ();

        Args::Arg & projection_step_param = arguments::get_projection_step ();

        Args::Arg & run_mode_param = arguments::get_run_mode();

        Args::Arg & interaction_mode_param = arguments::get_interaction_mode();

        Args::Arg & roof_position_walk_param = arguments::get_roof_position_walk ();

        Args::Arg & height_factor_param = arguments::get_height_factor ();

        Args::Arg & roof_variants_param = arguments::get_roof_varians();

        Args::Arg & shade_variants_param = arguments::get_shade_varians();

        Args::Arg & use_sam_param = arguments::get_use_sam ();

        Args::Help help;
        help.setAppDescription(
            SL( "Utility to reconstruct projection-shade buildings' structure from a vector of images and roof maps for the same territory."
                " Each object is saved into projes and shades datasets. "
                "The reconstruction can be performed in automatic mode with no user interaction, manual mode with total results revision "
                "and 'on demand' mode to solve complex issues. "
                "World coordinate systems for all input datasets should match otherwise the utility will fail (no reprojection or conversion is performed)." ) );
        help.setExecutable( argv[0] );

        cmd.addArg ( input_vector_param );
        cmd.addArg ( input_raster_param );
        cmd.addArg ( input_bounds_param );
        cmd.addArg ( output_param );
        cmd.addArg ( driver_param );
        cmd.addArg ( segmentize_step_param );
        cmd.addArg ( projection_step_param );
        cmd.addArg ( height_factor_param );
        cmd.addArg ( force_rewtire_param );
        cmd.addArg ( run_mode_param );
        cmd.addArg ( interaction_mode_param );
        cmd.addArg ( roof_position_walk_param );
        cmd.addArg ( roof_variants_param );
        cmd.addArg ( shade_variants_param );
        cmd.addArg ( use_sam_param );
        cmd.addArg ( help );

        cmd.parse();

        array_helper < std::string > iv_list_helper ( input_vector_param.value() );
        const bool input_vector_valid = iv_list_helper.verify ( std::cerr, "STOP: Input vector maps list is incorrect" );

        array_helper < std::string > ir_list_helper ( input_raster_param.value() );
        const bool input_raster_valid = ir_list_helper.verify ( std::cerr, "STOP: Input rasters list is incorrect" );

        array_helper < std::string > ib_list_helper ( input_bounds_param.value() );
        const bool input_bounds_valid = ib_list_helper.verify ( std::cerr, "STOP: Input vector maps list is incorrect" );

        array_helper < std::string > o_list_helper ( output_param.value() );
        const bool output_valid = o_list_helper.verify ( std::cerr, "STOP: Output directories list is incorrect" );

        if ( !input_vector_valid || !input_raster_valid || !output_valid || !input_bounds_valid )
        {
            std::cerr << "STOP: Input parameters verification failed" << std::endl;
            return 1;
        }

        auto input_vectors = iv_list_helper.value();
        shared_datasets ds_vectors = gdal::apply_helper < std::ostream, gdal::open_vector_ro_helper > ( input_vectors, std::cerr );

        auto input_rasters = ir_list_helper.value();
        shared_datasets ds_rasters = gdal::apply_helper < std::ostream, gdal::open_raster_ro_helper > ( input_rasters, std::cerr );

        auto input_bounds = ib_list_helper.value();
        shared_datasets ds_bounds = gdal::apply_helper < std::ostream, gdal::open_vector_ro_helper > ( input_bounds, std::cerr );

        auto outputs = o_list_helper.value();
        shared_datasets ds_outs = gdal::apply_helper < std::ostream, gdal::create_vector_helper > ( outputs, driver_param.value(), std::cerr );

        if ( !ds_vectors || !ds_rasters || !ds_outs || !ds_bounds )
        {
            std::cerr << "STOP: Input parameters verification failed" << std::endl;
            return 1;
        }

        if ( ds_vectors.size() != ds_rasters.size () )
        {
            std::cerr << "STOP: Number of input vectors should match the number of input rasters" << std::endl;
            return 1;
        }

        if ( ds_vectors.size() != ds_bounds.size () )
        {
            std::cerr << "STOP: Number of input vectors should match the number of bounds" << std::endl;
            return 1;
        }

        if ( ds_vectors.size() != ds_outs.size () )
        {
            std::cerr << "STOP: Number of input vectors should match the number of outputs" << std::endl;
            return 1;
        }

        // getting and validaring SRSs
        auto vec_srses = gdal::get_srses < std::ostream, gdal::open_vector_ro_helper > ( ds_vectors, std::cerr ),
             rst_srses = gdal::get_srses < std::ostream, gdal::open_raster_ro_helper > ( ds_rasters, std::cerr ),
             bnd_srses = gdal::get_srses < std::ostream, gdal::open_vector_ro_helper > ( ds_bounds, std::cerr );

        // Verifying vector and raster SRSs
        if ( !gdal::are_same ( vec_srses ) )
        {
            std::cerr << "STOP: Input vector maps' SRSs are not the same" << std::endl;
            return 1;
        }

        if ( !gdal::are_same ( bnd_srses ) )
        {
            std::cerr << "STOP: Input vector bounds maps' SRSs are not the same" << std::endl;
            return 1;
        }

        if ( vec_srses != rst_srses )
        {
            std::cerr << "STOP: Input vector maps' SRSs do not match corresponding raster SRSs" << std::endl;
            return 1;
        }

        // Building model segmentize step value
        value_helper < double > segmentize_step_helper      ( segmentize_step_param.value() );
        if ( !segmentize_step_helper.verify ( std::cerr,    "STOP: Building model segmentize step value is incorrect." ) )
            return 1;

        // Building model projection step value
        value_helper < int > projection_step_helper         ( projection_step_param.value() );
        if ( !projection_step_helper.verify ( std::cerr,    "STOP: Building model projection step value is incorrect." ) )
            return 1;

        // Building model projection step value
        value_helper < double > height_factor_helper        ( height_factor_param.value() );
        if ( !height_factor_helper.verify ( std::cerr,      "STOP: Orbital height factor is incorrect." ) )
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

        const double segmentize_step    = segmentize_step_helper.value();
        const int projection_step       = projection_step_helper.value();
        const double roof_position_walk = roof_position_walk_helper.value();
        const int roof_variants         = roof_variants_helper.value();
        const int shade_variants        = shade_variants_helper.value();
        const double height_factor      = height_factor_helper.value();

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

        rsai::multiview_building_reconstructor finder (
                                                ds_vectors
                                              , ds_rasters
                                              , ds_bounds
                                              , ds_outs
                                              , segmentize_step
                                              , projection_step
                                              , roof_position_walk
                                              , roof_variants
                                              , shade_variants
                                              , height_factor
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
