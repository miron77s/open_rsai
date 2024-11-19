#pragma once

#include <common/arguments.h>
#include <common/definitions.h>

template < class Dummy >
Args::Arg &arguments_t < Dummy >::get_input_vector()
{
    static Args::Arg input_vector_param( SL( 'v' ), SL( "input_vector" ), true, true );
    input_vector_param.setDescription( SL( "Input vector map in GDAL-supported format. " ) );

    return input_vector_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_input_raster ()
{
    static Args::Arg input_raster_param( SL( 'r' ), SL( "input_raster" ), true, true );
    input_raster_param.setDescription( SL( "Input raster in GDAL-supported format. " ) );

    return input_raster_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_input_ground_true ()
{
    static Args::Arg input_metadata_param( SL( 'g' ), SL( "ground_true" ), true, true );
    input_metadata_param.setDescription( SL( "Input ground true vector map. " ) );

    return input_metadata_param;
}

template < class Dummy >
Args::Arg &arguments_t < Dummy >::get_input_updating_map()
{
    static Args::Arg input_vector_param( SL( 'u' ), SL( "input_updating" ), true, true );
    input_vector_param.setDescription( SL( "Input vector map to be updated by a newer layer. " ) );

    return input_vector_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_input_metadata ()
{
    static Args::Arg input_metadata_param( SL( 'i' ), SL( "metadata" ), true, true );
    input_metadata_param.setDescription( SL( "Input metadata vector map. " ) );

    return input_metadata_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_input_denied_regions ()
{
    static Args::Arg input_denied_regions_param( SL( 'd' ), SL( "denied" ), true, false );
    input_denied_regions_param.setDescription( SL( "Input vector of denied polygons. " ) );

    return input_denied_regions_param;
}

template < class Dummy >
Args::Arg &arguments_t < Dummy >::get_input_bounds()
{
    static Args::Arg input_vector_param( SL( 'b' ), SL( "input_bounds" ), true, true );
    input_vector_param.setDescription( SL( "Input objects' bounds vector map in GDAL-supported format. " ) );

    return input_vector_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_output ()
{
    static Args::Arg output_param( SL( 'o' ), SL( "output" ), true, true );
    output_param.setDescription( SL( "Output directory to store the result. " ) );

    return output_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_output_optional ()
{
    static Args::Arg output_param( SL( 'o' ), SL( "output" ), true, false );
    output_param.setDescription( SL( "Output directory to store the result. " ) );

    return output_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_output_map ()
{
    static Args::Arg output_param( SL( 'm' ), SL( "output_map" ), true, false );
    output_param.setDescription( SL( "Output map to store additional the result. " ) );

    return output_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_driver ()
{
    static Args::Arg driver_param( SL( 'd' ), SL( "driver" ), true, false );
    driver_param.setDescription( std::string ( "GDAL driver key to save output vector map. "
                                                "Should be one of supported from https://gdal.org/drivers/vector/index.html. "
                                                "The default is " ) + DEFAULT_VECTOR_MAP_OUTPUT_DRIVER + ". " );
    driver_param.setDefaultValue ( DEFAULT_VECTOR_MAP_OUTPUT_DRIVER );

    return driver_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_force_rewtire ()
{
    static Args::Arg force_rewtire_param( SL( 'f' ), SL( "force_rewrite" ), false, false );
    force_rewtire_param.setDescription( SL( "Silently rewrites output dataset(s) if already exists. " ) );

    return force_rewtire_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_region_of_interest ()
{
    static Args::Arg roi_param( SL( 'i' ), SL( "roi" ), true, false );
    roi_param.setDescription( SL( "Input single-layer vector map containing region-of-interest polygon (only the first one would be applied). " ) );

    return roi_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_crop_by_raster ()
{
    static Args::Arg crop_geometry_param( SL( 'c' ), SL( "crop_by_raster" ), false, false );
    crop_geometry_param.setDescription( SL( "Crops output geometry using raster's bounding box. Default = false. " ) );

    return crop_geometry_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_semantic_filter ()
{
    static Args::Arg semantic_filter_param( SL( 's' ), SL( "semantic_filter" ), true, false );
    semantic_filter_param.setDescription( SL( "Use semantic values to filter the inliers like \"fclass = 'water' or fclass = 'reservior'\". " ) );

    return semantic_filter_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_min_raster_obj_size ()
{
    static Args::Arg max_proj_param( SL( 'm' ), SL( "min_raster_obj_size" ), true, false );
    max_proj_param.setDescription( std::string( "Minimal object size on the source raster to be copyed to the output layer. "
                                                " The default is " ) + DEFAULT_MIN_RASTER_OBJ_SIZE_VALUE + " m. " );
    max_proj_param.setDefaultValue ( DEFAULT_MIN_RASTER_OBJ_SIZE_VALUE );

    return max_proj_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_max_projection_length ()
{
    static Args::Arg max_proj_param( SL( 'l' ), SL( "max_proj_length" ), true, true );
    max_proj_param.setDescription( std::string( "Maximum projection vector length in world units (meters) to determine roof and shade positions. "
                                                "Used for performance optimizations. The default is " ) + DEFAULT_MAX_PROJECTION_LEN_VALUE + " m. " );
    max_proj_param.setDefaultValue ( DEFAULT_MAX_PROJECTION_LEN_VALUE );

    return max_proj_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_mask_size ()
{
    static Args::Arg mask_size_param( SL( 'm' ), SL( "mask" ), true, true );
    mask_size_param.setDescription( std::string( "Half value of convolutional filter's size (pixels) used to extract floor, roof and shade positions."
                                                 "Required to account it in raster bounds size and safely process the object's image avoiding the out-of-range errors. "
                                                 "The default is " ) + DEFAULT_SHADE_MASK_SIZE_VALUE + " pixels. " );
    mask_size_param.setDefaultValue ( DEFAULT_SHADE_MASK_SIZE_VALUE );

    return mask_size_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_segmentize_step ()
{
    static Args::Arg segmentize_step_param( SL( 's' ), SL( "segmentize_step" ), true, false );
    segmentize_step_param.setDescription( std::string ( "Step value to segmetize building model geometry (pixels). "
                                                        "The default is " ) + DEFAULT_SEGMENTIZE_STEP_VALUE + " pixels. " );
    segmentize_step_param.setDefaultValue ( DEFAULT_SEGMENTIZE_STEP_VALUE );

    return segmentize_step_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_projection_step ()
{
    static Args::Arg projection_step_param( SL( 'p' ), SL( "projection_step" ), true, false );
    projection_step_param.setDescription( std::string( "Step value for reconstruction of building model geometry (pixels). "
                                                       "The default is " ) + DEFAULT_PROJECTION_STEP_VALUE + " pixels. ");
    projection_step_param.setDefaultValue ( DEFAULT_PROJECTION_STEP_VALUE );

    return projection_step_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_run_mode ()
{
    static Args::Arg run_mode_param( SL( 'm' ), SL( "mode" ), true, false );
    run_mode_param.setDescription( std::string ( "Utility run mode. 'auto' - no user interaction / silent mode, 'manual' - aligning every object, "
                                                 "'on_demand' - aligning nontrivial cases. "
                                                 "The default is '" ) + DEFAULT_RECONSTRUCTION_MODE + "'. " );
    run_mode_param.setDefaultValue ( DEFAULT_RECONSTRUCTION_MODE );
    return run_mode_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_interaction_mode ()
{
    static Args::Arg interaction_mode_param( SL( 'n' ), SL( "interaction_type" ), true, false );
    interaction_mode_param.setDescription( std::string( "User interaction type for objects aligning: 'internal' - use internal OpenCV-based interface, "
                                                        "'external' - save intermediate data (source, roofs, projections images and wkt polygons) for external interface. "
                                                        "The default is '" ) + DEFAULT_INTERACTION_MODE + "'. " );
    interaction_mode_param.setDefaultValue ( DEFAULT_INTERACTION_MODE );

    return interaction_mode_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_roof_position_walk ()
{
    static Args::Arg roof_position_walk_param( SL( 'w' ), SL( "roof_position_walk" ), true, false );
    roof_position_walk_param.setDescription( std::string( "Maximum offset value of the roof contour to find it's position on image (pixels). "
                                                          "The default is " ) + DEFAULT_ROOF_POSITION_WALK_VALUE + " pixels. ");
    roof_position_walk_param.setDefaultValue ( DEFAULT_ROOF_POSITION_WALK_VALUE );

    return roof_position_walk_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_height_factor ()
{
    static Args::Arg height_factor_param( SL( 'h' ), SL( "height_factor" ), true, false );
    height_factor_param.setDescription( std::string( "Height factor defined by orbital params on shooting in m/pixel. "
                                                          "The default is " ) + DEFAULT_HEIGHT_FACTOR_VALUE + " m/pixel. ");
    height_factor_param.setDefaultValue ( DEFAULT_HEIGHT_FACTOR_VALUE );

    return height_factor_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_roof_varians ()
{
    static Args::Arg roof_varians_param( SL( 'k' ), SL( "roof_varians" ), true, false );
    roof_varians_param.setDescription( std::string( "Quantity of roof position variants to generate. "
                                                    "The default is " ) + DEFAULT_ROOF_VARIANTS_VALUE + " variants. ");
    roof_varians_param.setDefaultValue ( DEFAULT_ROOF_VARIANTS_VALUE );

    return roof_varians_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_shade_varians ()
{
    static Args::Arg shade_varians_param( SL( 'l' ), SL( "shade_varians" ), true, false );
    shade_varians_param.setDescription( std::string( "Quantity of shade position variants to generate per each position variant. "
                                                    "The default is " ) + DEFAULT_SHADE_VARIANTS_VALUE + " variants. ");
    shade_varians_param.setDefaultValue ( DEFAULT_SHADE_VARIANTS_VALUE );

    return shade_varians_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_tile_buffer_size ()
{
    static Args::Arg tile_buffer_size_param( SL( 't' ), SL( "tile_buffer_size" ), true, false );
    tile_buffer_size_param.setDescription( std::string( "Image tile buffer's size to be enlarged from object's envelope (pixels). "
                                                        "The default is " ) + DEFAULT_SEG_ANY_TILE_BUFFER_VALUE + " pixels. " );
    tile_buffer_size_param.setDefaultValue ( DEFAULT_SEG_ANY_TILE_BUFFER_VALUE );

    return tile_buffer_size_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_use_sam ()
{
    static Args::Arg use_sam_param( SL( "use_sam" ), false, false );
    use_sam_param.setDescription( SL ( "If defined enables utility to use Segment Anything's objects boundaries to reconstruct buiding's structure. " ) );
    return use_sam_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_min_first_pos_weight ()
{
    static Args::Arg min_first_pos_weight_param( SL( "max_first_pos_weight" ), true, false );
    min_first_pos_weight_param.setDescription( std::string( "Minimum roof's first position weight to run automatically in on_demand mode. "
                                                        "The default is " ) + DEFAULT_MIN_FIRST_POS_WEIGHT + " pixels. " );
    min_first_pos_weight_param.setDefaultValue ( DEFAULT_MIN_FIRST_POS_WEIGHT );

    return min_first_pos_weight_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_max_first_pos_deviation ()
{
    static Args::Arg max_first_pos_deviation_param( SL( "max_first_pos_deviation" ), true, false );
    max_first_pos_deviation_param.setDescription( std::string( "Maximum roof's first position deviation to run automatically in on_demand mode. "
                                                        "The default is " ) + DEFAULT_MAX_FIRST_POS_DEVIATION + " pixels. " );
    max_first_pos_deviation_param.setDefaultValue ( DEFAULT_MAX_FIRST_POS_DEVIATION );

    return max_first_pos_deviation_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_markup_tile_sizes ()
{
    static Args::Arg markup_tile_sizes_param( SL ( 't' ), SL( "tile_sizes" ), true, false );
    markup_tile_sizes_param.setDescription( std::string( "Output raster tile sizes in format 'X,Y' to generate markup. "
                                                        "The default is " ) + DEFAULT_MARKUP_TILE_SIZES + " pixels. " );
    markup_tile_sizes_param.setDefaultValue ( DEFAULT_MARKUP_TILE_SIZES );

    return markup_tile_sizes_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_markup_classes ()
{
    static Args::Arg markup_classes_param( SL ( 'c' ), SL( "classes" ), true, false );
    markup_classes_param.setDescription( std::string( "A list of comma separated class names to create a markup. Should match vector layer names. "
                                                        "The default list is '" ) + DEFAULT_MARKUP_CLASSES + "'. " );
    markup_classes_param.setDefaultValue ( DEFAULT_MARKUP_CLASSES );

    return markup_classes_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_markup_balance ()
{
    static Args::Arg markup_balance_param( SL ( 'b' ), SL( "balance" ), true, false );
    markup_balance_param.setDescription( std::string( "A balance value [0..1] between empty and nonempty tiles. Represents nonempty tiles part in the output dataset. "
                                                        "The default value is " ) + DEFAULT_MARKUP_BALANCE + ". " );
    markup_balance_param.setDefaultValue ( DEFAULT_MARKUP_BALANCE );

    return markup_balance_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_markup_validation ()
{
    static Args::Arg markup_validation_param( SL( "validation" ), true, false );
    markup_validation_param.setDescription( std::string( "A dataset part [0.." ) + std::to_string ( MAXIMUM_MARKUP_VALIDATION ) + "] to be used for validation (not training). "
                                                        "The default value is " + DEFAULT_MARKUP_VALIDATION + ". " );
    markup_validation_param.setDefaultValue ( DEFAULT_MARKUP_VALIDATION );

    return markup_validation_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_markup_overlap ()
{
    static Args::Arg markup_overlap_param( SL( "overlap" ), true, false );
    markup_overlap_param.setDescription( std::string( "Output raster tile overlap factor. "
                                                        "The default value is " ) + DEFAULT_MARKUP_OVERLAP + ". " );
    markup_overlap_param.setDefaultValue ( DEFAULT_MARKUP_OVERLAP );

    return markup_overlap_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_markup_format ()
{
    static Args::Arg markup_format_param( SL( "format" ), true, false );
    markup_format_param.setDescription( std::string( "A target format to save markup. "
                                                        "The default value is " ) + DEFAULT_MARKUP_FORMAT + ". " );
    markup_format_param.setDefaultValue ( DEFAULT_MARKUP_FORMAT );

    return markup_format_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_iou_match_thresh ()
{
    static Args::Arg iou_match_thresh_param( SL( "iou_match_thresh" ), true, false );
    iou_match_thresh_param.setDescription( std::string ( "An IoU value threshold used to separate changed and unchanged objects. "
                                                        "The default is " ) + DEFAULT_IOU_MATCH_THRESH_VALUE + ". " );
    iou_match_thresh_param.setDefaultValue ( DEFAULT_IOU_MATCH_THRESH_VALUE );

    return iou_match_thresh_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_iou_min_thresh ()
{
    static Args::Arg iou_min_thresh_param( SL( "iou_min_thresh" ), true, false );
    iou_min_thresh_param.setDescription( std::string ( "An IoU value threshold used to decide wherever a colision is reasonable. "
                                                        "The default is " ) + DEFAULT_IOU_MIN_THRESH_VALUE + ". " );
    iou_min_thresh_param.setDefaultValue ( DEFAULT_IOU_MIN_THRESH_VALUE );

    return iou_min_thresh_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_save_update_diff ()
{
    static Args::Arg save_update_diff_param( SL( "save_diff" ), false, false );
    save_update_diff_param.setDescription( std::string ( "Save updating newer objects to '" ) + DEFAULT_UPCOMMING_LAYER_NAME
                                           + "' and outdated objects to '" DEFAULT_OUTDATED_LAYER_NAME + "' maps. " );

    return save_update_diff_param;
}

template < class Dummy >
Args::Arg & arguments_t < Dummy >::get_save_updated_map ()
{
    static Args::Arg save_updated_map_param( SL( "save_updated" ), false, false );
    save_updated_map_param.setDescription( std::string ( "Save updated map to a '") + DEFAULT_UPDATED_LAYER_NAME + "' map. " );

    return save_updated_map_param;
}
