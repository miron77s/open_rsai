#pragma once

#include <string>
#include <Eigen/Dense>

#include "common/promt_functions.hpp"
#include "common/progress_functions.hpp"
#include "common/string_utils.h"
#include "rsai/markup/writers.h"

namespace rsai
{

    class markup_writer
    {
    public:
        template < class PromtFunc, class ProgressFunc >
        markup_writer  (
                                          const strings &vector_maps
                                        , gdal::shared_datasets &ds_rasters
                                        , gdal::shared_dataset ds_denied
                                        , const std::string &output_dir
                                        , const bool force_rewrite
                                        , const double balance
                                        , const double overlap
                                        , const double validation
                                        , const Eigen::Vector2i tile_sizes
                                        , const strings &classes
                                        , const rsai::markup::markup_type &format
                                        , const write_mode &mode
                                        , const PromtFunc &promt_func = rewrite_layer_promt_dummy
                                        , const ProgressFunc &progress_func = progress_dummy
                                      );
    }; // class markup_writer

    write_mode write_mode_from_string ( const std::string &mode );
    rsai::markup::markup_type markup_type_from_string ( const std::string &type );
}; // namespace rsai

#include "rsai/markup_writer.hpp"
