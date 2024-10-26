#include <iostream>
#include <string>

#include <args-parser/all.hpp>

#include "common/definitions.h"
#include "common/promt_functions.hpp"
#include "common/arguments.h"
#include "rsai/map_updater.h"
#include "common/string_utils.h"

using namespace std;

int main ( int argc, char * argv[] )
{
    try
    {
        Args::CmdLine cmd( argc, argv );

        Args::Arg &input_vector_param = arguments::get_input_vector ();
        input_vector_param.setDescription( SL( "Input vector map in GDAL-supported format to update an existing map. " ) );

        Args::Arg & input_updating_map_param = arguments::get_input_updating_map ();

        Args::Arg & input_updating_roi_param = arguments::get_region_of_interest ();

        Args::Arg & output_param = arguments::get_output ();

        Args::Arg & driver_param = arguments::get_driver ();

        Args::Arg & force_rewtire_param = arguments::get_force_rewtire ();

        Args::Arg & iou_match_thresh_param = arguments::get_iou_match_thresh ();

        Args::Arg & iou_min_thresh_param = arguments::get_iou_min_thresh ();

        Args::Arg & save_update_diff_param = arguments::get_save_update_diff ();

        Args::Arg & save_updated_map_param = arguments::get_save_updated_map ();

        Args::Help help;
        help.setAppDescription(
            std::string ( "Utility to update an existsing map with object from a new map. " ) );
        help.setExecutable( argv[0] );

        cmd.addArg ( input_vector_param );
        cmd.addArg ( input_updating_map_param );
        cmd.addArg ( input_updating_roi_param );
        cmd.addArg ( output_param );
        cmd.addArg ( driver_param );
        cmd.addArg ( force_rewtire_param );
        cmd.addArg ( iou_match_thresh_param );
        cmd.addArg ( iou_min_thresh_param );
        cmd.addArg ( save_update_diff_param );
        cmd.addArg ( save_updated_map_param );
        cmd.addArg ( help );

        cmd.parse();

        gdal::open_vector_ro_helper iv_helper ( input_vector_param.value(), std::cerr );
        auto ds_vector = iv_helper.validate ( true );

        gdal::open_vector_ro_helper iv_updated_helper ( input_updating_map_param.value(), std::cerr );
        auto ds_updated = iv_updated_helper.validate ( true );

        gdal::open_vector_ro_helper iv_roi_helper ( input_updating_roi_param.value(), std::cerr );
        auto ds_roi = iv_roi_helper.validate ( true );

        gdal::create_vector_helper out_helper ( output_param.value(), driver_param.value(), std::cerr );
        auto ds_out = out_helper.validate ( true );

        // getting and validaring SRSs
        auto vec_srs = iv_helper.srs(),
             updated_srs = iv_updated_helper.srs(),
             roi_srs = iv_roi_helper.srs();

        // Verifying SRSs
        if ( !vec_srs.verify ( std::cerr ) || !updated_srs.verify ( std::cerr ) || !roi_srs.verify ( std::cerr ) )
        {
            std::cerr << "STOP: Input datasets' SRS are not valid" << std::endl;
            return 1;
        }

        value_helper < double > iou_match_thresh_helper      ( iou_match_thresh_param.value() );
        if ( !iou_match_thresh_helper.verify ( std::cerr,    "STOP: IoU match threshold value is incorrect." ) )
            return 1;

        value_helper < double > iou_min_thresh_helper      ( iou_match_thresh_param.value() );
        if ( !iou_min_thresh_helper.verify ( std::cerr,    "STOP: IoU min threshold value is incorrect." ) )
            return 1;


        rsai::map_updater map_updater (
                                          ds_vector
                                        , ds_updated
                                        , ds_roi
                                        , ds_out
                                        , iou_match_thresh_helper.value()
                                        , iou_min_thresh_helper.value()
                                        , save_update_diff_param.isDefined()
                                        , save_updated_map_param.isDefined()
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
