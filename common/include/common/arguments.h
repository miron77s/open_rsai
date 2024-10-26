#pragma once

#include <args-parser/all.hpp>

template < class Dummy = int >
class arguments_t
{
public:
    static Args::Arg & get_input_vector ();
    static Args::Arg & get_input_raster ();
    static Args::Arg & get_input_metadata ();
    static Args::Arg & get_input_denied_regions ();
    static Args::Arg & get_input_ground_true ();
    static Args::Arg & get_input_updating_map ();
    static Args::Arg & get_input_bounds ();
    static Args::Arg & get_output ();
    static Args::Arg & get_output_optional ();
    static Args::Arg & get_output_map ();
    static Args::Arg & get_driver ();

    static Args::Arg & get_force_rewtire ();

    static Args::Arg & get_region_of_interest ();
    static Args::Arg & get_crop_by_raster ();
    static Args::Arg & get_semantic_filter ();
    static Args::Arg & get_min_raster_obj_size ();

    static Args::Arg & get_max_projection_length ();
    static Args::Arg & get_mask_size ();

    static Args::Arg & get_segmentize_step ();
    static Args::Arg & get_projection_step ();
    static Args::Arg & get_run_mode ();
    static Args::Arg & get_interaction_mode ();
    static Args::Arg & get_roof_position_walk ();
    static Args::Arg & get_roof_varians ();
    static Args::Arg & get_shade_varians ();
    static Args::Arg & get_use_sam ();

    static Args::Arg & get_tile_buffer_size ();
    static Args::Arg & get_min_first_pos_weight ();
    static Args::Arg & get_max_first_pos_deviation ();

    static Args::Arg & get_markup_tile_sizes ();
    static Args::Arg & get_markup_classes ();
    static Args::Arg & get_markup_balance ();
    static Args::Arg & get_markup_validation ();
    static Args::Arg & get_markup_overlap ();
    static Args::Arg & get_markup_format ();

    static Args::Arg & get_iou_match_thresh ();
    static Args::Arg & get_iou_min_thresh ();
    static Args::Arg & get_save_update_diff ();
    static Args::Arg & get_save_updated_map ();
};

using arguments = arguments_t <>;

#include <common/arguments.hpp>
