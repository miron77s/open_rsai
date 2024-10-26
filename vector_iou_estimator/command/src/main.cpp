#include <iostream>
#include <string>

#include <args-parser/all.hpp>

#include "common/definitions.h"
#include "common/promt_functions.hpp"
#include "common/arguments.h"
#include "rsai/vector_iou_estimator.h"
#include "common/string_utils.h"

using namespace std;

int main ( int argc, char * argv[] )
{
    try
    {
        Args::CmdLine cmd( argc, argv );

        Args::Arg &input_vector_param = arguments::get_input_vector ();
        input_vector_param.setDescription( SL( "A comma-separated list of input vector maps' file names to match with the list of ground true maps. " ) );

        Args::Arg & input_ground_true_param = arguments::get_input_ground_true ();
        input_ground_true_param.setDescription( input_ground_true_param.description() + SL ( "A comma-separated list of ground true maps maps' file names to match to match with input maps. " ) );

        Args::Arg & output_optional_param = arguments::get_output_optional ();
        output_optional_param.setDescription( output_optional_param.description() + "The detailed IoU report for each input layer will saved if this parameter is set." );

        Args::Arg & driver_param = arguments::get_driver ();

        Args::Arg & force_rewtire_param = arguments::get_force_rewtire ();

        Args::Help help;
        help.setAppDescription(
            std::string ( "Utility to calculate IoU via vector 2 ground true matching ( object against best ground true object ). " ) );
        help.setExecutable( argv[0] );

        cmd.addArg ( input_vector_param );
        cmd.addArg ( input_ground_true_param );
        cmd.addArg ( output_optional_param );
        cmd.addArg ( driver_param );
        cmd.addArg ( force_rewtire_param );
        cmd.addArg ( help );

        cmd.parse();

        array_helper < std::string > iv_list_helper ( input_vector_param.value() );
        const bool input_vector_valid = iv_list_helper.verify ( std::cerr, "STOP: Input vector maps list is incorrect" );

        array_helper < std::string > ig_list_helper ( input_ground_true_param.value() );
        const bool input_ground_true_valid = ig_list_helper.verify ( std::cerr, "STOP: Input ground true maps list is incorrect" );

        array_helper < std::string > o_list_helper ( output_optional_param.value() );
        const bool output_valid = o_list_helper.verify ( std::cerr, "STOP: Output directories list is incorrect" );

        if ( !input_vector_valid || !input_ground_true_valid || !output_valid )
        {
            std::cerr << "STOP: Input parameters verification failed" << std::endl;
            return 1;
        }

        auto input_vectors = iv_list_helper.value();
        shared_datasets ds_vectors = gdal::apply_helper < std::ostream, gdal::open_vector_ro_helper > ( input_vectors, std::cerr );

        auto input_ground_trues = ig_list_helper.value();
        shared_datasets ds_ground_trues = gdal::apply_helper < std::ostream, gdal::open_vector_ro_helper > ( input_ground_trues, std::cerr );

        auto outputs = o_list_helper.value();
        shared_datasets ds_outs = gdal::apply_helper < std::ostream, gdal::create_vector_helper > ( outputs, driver_param.value(), std::cerr );

        if ( !ds_vectors || !ds_ground_trues || !ds_outs )
        {
            std::cerr << "STOP: Input parameters verification failed" << std::endl;
            return 1;
        }

        // getting and validaring SRSs
        auto vec_srses = gdal::get_srses < std::ostream, gdal::open_vector_ro_helper > ( ds_vectors, std::cerr ),
             gt_srses = gdal::get_srses < std::ostream, gdal::open_vector_ro_helper > ( ds_ground_trues, std::cerr );

        // Verifying vector and raster SRSs
        if ( !gdal::are_same ( vec_srses ) )
        {
            std::cerr << "STOP: Input vector maps' SRSs are not the same" << std::endl;
            return 1;
        }

        if ( vec_srses != gt_srses [0] || gt_srses != vec_srses [0] )
        {
            std::cerr << "STOP: Input vector maps' SRSs do not match ground true maps' SRSs" << std::endl;
            return 1;
        }

        rsai::vector_iou_estimator vector_iou_estimator (
                                                              ds_vectors
                                                            , ds_ground_trues
                                                            , ds_outs
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
