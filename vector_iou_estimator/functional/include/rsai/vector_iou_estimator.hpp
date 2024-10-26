#pragma once

#include "rsai/vector_iou_estimator.h"

#include <ogrsf_frmts.h>

#include "gdal_utils/all_helpers.h"
#include "gdal_utils/shared_options.h"
#include "gdal_utils/region_of_interest.h"
#include "gdal_utils/shared_feature.h"
#include "gdal_utils/layers.h"
#include "threading_utils/gdal_iterators.h"
#include "gdal_utils/operations.h"

#include "common/definitions.h"

template < class PromtFunc, class ProgressFunc >
rsai::vector_iou_estimator::vector_iou_estimator (
                                                    gdal::shared_datasets &ds_vectors
                                                  , gdal::shared_datasets &ds_ground_trues
                                                  , gdal::shared_datasets &ds_outs
                                                  , const PromtFunc &promt_func
                                                  , const ProgressFunc &progress_func
                                                 )
{
    if ( ds_vectors.size () == 0 )
        return;

    static const std::string id_field_name = DEFAULT_OBJECT_ID_FIELD_NAME;

    gdal::ogr_layers gt_layers;
    for ( auto ds_ground_true : ds_ground_trues )
    {
        for( int i = 0; i < ds_ground_true->GetLayerCount (); ++i )
            gt_layers.add ( ds_ground_true->GetLayer ( i ) );
    }

    int total_layers = 0;
    for ( auto ds_vector : ds_vectors )
        total_layers += ds_vector->GetLayerCount ();
    int total_layer = 0;

    std::mutex locker;
    std::pair < double, int > ious ( 0.0, 0 );

    std::string group_name;
    std::string base_name;

    bool layers_result = true;
    for ( auto ds_vector : ds_vectors )
    {
        const auto vector_path = ds_vector->GetDescription();
        std::filesystem::path path (vector_path);
        const auto vector_file_name = path.stem().string();
        group_name = path.parent_path().filename().string();
        base_name  = path.parent_path().parent_path().filename().string();

        std::pair < double, int > layer_ious ( 0.0, 0 );

        const int layers = ds_vector->GetLayerCount ();
        for( int i = 0; i < layers; ++i )
        {
            auto layer = ds_vector->GetLayer ( i );

            threading::layer_iterator a_layer_iterator ( layer );
            layers_result &= a_layer_iterator ( [&] ( gdal::shared_feature feature, const int current_feature_id )
            {
                OGRGeometry *geometry = feature->GetGeometryRef ();

                gt_layers.set_spatial_filter ( geometry );
                auto gt_features = gt_layers.search();

                double best_iou = 0.0;
                for ( auto gt_feature : gt_features )
                {
                    auto iou_value = gdal::iou ( geometry, gt_feature->GetGeometryRef() );
                    best_iou = std::max ( best_iou, iou_value );
                }

                std::scoped_lock lock ( locker );
                layer_ious.first += best_iou;
                ++layer_ious.second;

            }, total_layer++, total_layers, progress_layers_dummy, 1 );
        }

        //std::cout << "Layer " << group_name << "/" << vector_file_name << ", objects " << layer_ious.second
        //          << " -> IoU = " << layer_ious.first / layer_ious.second * 100. << "%" << '\n';

        ious.first += layer_ious.first;
        ious.second += layer_ious.second;
    }

    std::cout << base_name << "/" << group_name << ", objects " << ious.second << " IoU = " << ious.first / ious.second * 100. << "%" << '\n';
}
