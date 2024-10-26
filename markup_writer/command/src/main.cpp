#include <iostream>
#include <string>

#include <args-parser/all.hpp>

#include "rsai/markup_writer.h"

#include "common/definitions.h"
#include "common/arguments.h"
#include "common/promt_functions.hpp"
#include "common/progress_functions.hpp"
#include "eigen_utils/vector_helpers.h"

using namespace std;

int main ( int argc, char * argv[] )
{
    try
    {
        Args::CmdLine cmd( argc, argv );

        Args::Arg &input_vector_param = arguments::get_input_vector ();
        input_vector_param.setDescription( SL( "A comma-separated list of input vector maps' root directories. Each map stores objects' geometries. "
                                               "Used to generate output markup." ) );

        Args::Arg & input_raster_param = arguments::get_input_raster ();
        input_raster_param.setDescription( SL ( "A comma-separated list of input rasters' file names. Used to split into tiles and save according the choosen balance. "
                                           "Rasters are used to find actual buildings' shades and projection boundaries." ) );

        Args::Arg & input_denied_regions_param  = arguments::get_input_denied_regions ();

        Args::Arg & output_param = arguments::get_output ();
        output_param.setDescription( SL ( "An output directory to generator or append existing markup." ) );

        Args::Arg & force_rewtire_param = arguments::get_force_rewtire ();

        Args::Arg & markup_tile_sizes_param = arguments::get_markup_tile_sizes ();

        Args::Arg & markup_classes_param = arguments::get_markup_classes ();

        Args::Arg & markup_balance_param = arguments::get_markup_balance ();

        Args::Arg & markup_validation_param = arguments::get_markup_validation ();

        Args::Arg & markup_overlap_param = arguments::get_markup_overlap ();

        Args::Arg & markup_format_param = arguments::get_markup_format ();

        Args::Arg & run_mode_param = arguments::get_run_mode ();
        run_mode_param.setDescription( std::string ( "Markup save mode. Can be " ) + "'" + DEFAULT_MARKUP_APPEND_MODE + "'" + " to append an existing dataset with new data. "
                                       + "Or instead use " + "'" + DEFAULT_MARKUP_REPLACE_MODE + "'" + " to rewrite." );

        Args::Help help;
        help.setAppDescription(
            SL( "Utility to create or append to output dataset." ) );
        help.setExecutable( argv[0] );

        cmd.addArg ( input_vector_param );
        cmd.addArg ( input_raster_param );
        cmd.addArg ( input_denied_regions_param );
        cmd.addArg ( output_param );
        cmd.addArg ( force_rewtire_param );
        cmd.addArg ( markup_tile_sizes_param );
        cmd.addArg ( markup_classes_param );
        cmd.addArg ( markup_balance_param );
        cmd.addArg ( markup_validation_param );
        cmd.addArg ( markup_overlap_param );
        cmd.addArg ( markup_format_param );
        cmd.addArg ( run_mode_param );
        cmd.addArg ( help );

        cmd.parse();

        array_helper < std::string > iv_list_helper ( input_vector_param.value() );
        const bool input_vector_valid = iv_list_helper.verify ( std::cerr, "STOP: Input vector maps list is incorrect" );

        array_helper < std::string > ir_list_helper ( input_raster_param.value() );
        const bool input_raster_valid = ir_list_helper.verify ( std::cerr, "STOP: Input rasters list is incorrect" );

        gdal::open_vector_ro_helper denied_helper ( input_denied_regions_param.value(), std::cerr );
        auto ds_denied = denied_helper.validate ( false );

        array_helper < std::string > classes_list_helper ( markup_classes_param.value() );
        const bool classes_list_valid = classes_list_helper.verify ( std::cerr, "STOP: Dataset classes list is incorrect" );

        if ( !input_vector_valid || !input_raster_valid || !classes_list_valid )
        {
            std::cerr << "STOP: Input parameters verification failed" << std::endl;
            return 1;
        }

        auto input_vectors = iv_list_helper.value();

        auto input_rasters = ir_list_helper.value();
        shared_datasets ds_rasters = gdal::apply_helper < std::ostream, gdal::open_raster_ro_helper > ( input_rasters, std::cerr );

        if ( !input_vectors.size() || !ds_rasters || ds_denied == nullptr && !input_denied_regions_param.value().empty () )
        {
            std::cerr << "STOP: Input parameters verification failed" << std::endl;
            return 1;
        }

        // Building model segmentize step value
        value_helper < double > markup_balance_helper      ( markup_balance_param.value() );
        if ( !markup_balance_helper.verify ( std::cerr,    "STOP: Markup balance value is incorrect." ) )
            return 1;

        // Building model segmentize step value
        value_helper < double > markup_validation_helper   ( markup_validation_param.value() );
        if ( !markup_validation_helper.verify ( std::cerr, "STOP: Markup validation part value is incorrect." ) )
            return 1;

        if ( markup_validation_helper.value() > MAXIMUM_MARKUP_VALIDATION )
        {
            std::cerr << "STOP: Markup validation part value cannot be greater " << MAXIMUM_MARKUP_VALIDATION << '\n';
            return 1;
        }

        value_helper < double > markup_overlap_helper      ( markup_overlap_param.value() );
        if ( !markup_overlap_helper.verify ( std::cerr,    "STOP: Tiles overlap value is incorrect." ) )
            return 1;

        const auto run_mode = rsai::write_mode_from_string( run_mode_param.value() );
        if ( run_mode == write_mode::invalid )
        {
            std::cerr << "STOP: Write dataset mode '" << run_mode_param.value() << "' is invalid. Use --help param for correct values." << std::endl;
            return 1;
        }

        const auto markup_format = rsai::markup_type_from_string( markup_format_param.value() );
        if ( markup_format == rsai::markup::markup_type::invalid )
        {
            std::cerr << "STOP: Markup format '" << markup_format_param.value() << "' is invalid. Use --help param for correct values." << std::endl;
            return 1;
        }

        eigen::vector_helper < int, 2 > markup_tile_sizes_helper ( markup_tile_sizes_param.value(), std::string ( DEFAULT_DELIMITERS ) + "x" );
        if ( !markup_tile_sizes_helper.verify ( std::cerr ) )
        {
            std::cerr << "STOP: Failed to parse raster tile sizes. " << std::endl;
            return 1;
        }



        rsai::markup_writer writer (
                                            input_vectors
                                          , ds_rasters
                                          , ds_denied
                                          , output_param.value()
                                          , force_rewtire_param.isDefined()
                                          , markup_balance_helper.value()
                                          , markup_overlap_helper.value()
                                          , markup_validation_helper.value()
                                          , markup_tile_sizes_helper.value()
                                          , classes_list_helper.value()
                                          , markup_format
                                          , run_mode
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
