#pragma once

#include "rsai/map_updater.h"

#include <ogrsf_frmts.h>

#include "gdal_utils/all_helpers.h"
#include "gdal_utils/shared_options.h"
#include "gdal_utils/region_of_interest.h"
#include "gdal_utils/shared_feature.h"
#include "gdal_utils/layers.h"
#include "threading_utils/gdal_iterators.h"
#include "gdal_utils/operations.h"

#include "common/definitions.h"

struct features_with_iou
{
    gdal::shared_feature feature;
    double iou = 0;

    bool operator > ( const features_with_iou &rh ) { return iou > rh.iou; }
};

std::list < gdal::shared_feature > get_difference ( gdal::ogr_layer * verified, gdal::ogr_layer * base, OGRGeometry * update_region, const double iou_match_thresh, const double iou_min_thresh )
{
    verified->SetSpatialFilter ( update_region );
    verified->ResetReading ();

    std::list < gdal::shared_feature > result;

    threading::layer_iterator a_layer_iterator ( verified );
    auto layer_result = a_layer_iterator ( [&] ( gdal::shared_feature feature, const int current_feature_id )
    {
        OGRGeometry *geometry = feature->GetGeometryRef ();

        base->SetSpatialFilter ( geometry );
        base->ResetReading();

        std::vector < features_with_iou > collisions;
        while ( gdal::shared_feature older_feature = base->GetNextFeature() )
        {
            auto iou_value = gdal::iou ( geometry, older_feature->GetGeometryRef() );
;
            if ( iou_value > iou_min_thresh )
                collisions.push_back ( { older_feature, iou_value } );
        }

        // Object is almost the same
        if ( collisions.size() > 0 )
        {

            if ( collisions.front().iou > iou_match_thresh && collisions.size() == 1 )
                return;
            else
                result.push_back ( feature );
        }
        else
            result.push_back ( feature );

    }, 0, 1, console_progress_layers, 1 );

    return std::move ( result );
}

std::list < gdal::shared_feature > get_matching ( gdal::ogr_layer * verified, gdal::ogr_layer * base, const double iou_match_thresh, const double iou_min_thresh )
{
    verified->SetSpatialFilter ( nullptr );
    verified->ResetReading ();

    std::list < gdal::shared_feature > result;

    threading::layer_iterator a_layer_iterator ( verified );
    auto layer_result = a_layer_iterator ( [&] ( gdal::shared_feature feature, const int current_feature_id )
    {
        OGRGeometry *geometry = feature->GetGeometryRef ();

        base->SetSpatialFilter ( geometry );
        base->ResetReading();

        std::vector < features_with_iou > collisions;
        while ( gdal::shared_feature older_feature = base->GetNextFeature() )
        {
            auto iou_value = gdal::iou ( geometry, older_feature->GetGeometryRef() );
;
            if ( iou_value > iou_min_thresh )
                collisions.push_back ( { older_feature, iou_value } );
        }

        // Object is almost the same
        if ( collisions.size() > 0 )
        {
            if ( collisions.front().iou > iou_match_thresh && collisions.size() == 1 )
                result.push_back ( feature );
        }

    }, 0, 1, console_progress_layers, 1 );

    return std::move ( result );
}

template < class PromtFunc, class ProgressFunc >
rsai::map_updater::map_updater (
                                                     gdal::shared_dataset &ds_vector
                                                   , gdal::shared_dataset &ds_updating
                                                   , gdal::shared_dataset &ds_roi
                                                   , gdal::shared_dataset &ds_out
                                                   , const double iou_match_thresh
                                                   , const double iou_min_thresh
                                                   , const bool save_difference
                                                   , const bool save_updated
                                                   , const PromtFunc &promt_func
                                                   , const ProgressFunc &progress_func
                                                 )
{
    // Updated features region
    gdal::region_of_interest roi_wrp ( ds_roi );
    auto roi = roi_wrp.first_found ();

    const int layers = ds_vector->GetLayerCount ();
    for( int i = 0; i < layers; ++i )
    {
        auto new_layer = ds_vector->GetLayer ( i );
        auto updating_layer = ds_updating->GetLayer ( i );

        std::cout << "Finding new objects...\n";
        auto upcomming = get_difference ( new_layer, updating_layer, roi.get(), iou_match_thresh, iou_min_thresh );

        std::cout << "Finding outdated objects...\n";
        auto outdated = get_difference ( updating_layer, new_layer, roi.get(), iou_match_thresh, iou_min_thresh );

        std::cout << "Finding remaining objects...\n";
        auto retained = get_matching ( updating_layer, new_layer, iou_match_thresh, iou_min_thresh );

        std::cout << "Retained " << retained.size () << " objects, deleted " << outdated.size() << " created " << upcomming.size () << '\n';

        if ( save_difference )
        {
            gdal::create_vector_helper layers_helper ( ds_out, std::cerr );
            auto outdated_layer = layers_helper.create_layer ( DEFAULT_OUTDATED_LAYER_NAME, updating_layer->GetGeomType (), updating_layer->GetSpatialRef()
                                                               , true, promt_func );
            if ( !outdated_layer )
                continue;

            for ( auto feature : outdated )
            {
                if( outdated_layer->CreateFeature( feature.get () ) != OGRERR_NONE )
                {
                    std::cerr << "Stop: Failed to create feature in layer " << DEFAULT_OUTDATED_LAYER_NAME << std::endl;
                    return;
                }
            }

            auto upcomming_layer = layers_helper.create_layer ( DEFAULT_UPCOMMING_LAYER_NAME, updating_layer->GetGeomType (), updating_layer->GetSpatialRef()
                                                               , true, promt_func );
            if ( !upcomming_layer )
                continue;

            for ( auto feature : upcomming )
            {
                if( upcomming_layer->CreateFeature( feature.get () ) != OGRERR_NONE )
                {
                    std::cerr << "Stop: Failed to create feature in layer " << DEFAULT_UPCOMMING_LAYER_NAME << std::endl;
                    return;
                }
            }
        }

        if ( save_updated )
        {
            gdal::create_vector_helper layers_helper ( ds_out, std::cerr );
            auto updated_layer = layers_helper.create_layer ( DEFAULT_UPDATED_LAYER_NAME, updating_layer->GetGeomType (), updating_layer->GetSpatialRef()
                                                              , true, promt_func );

            for ( auto feature : upcomming )
            {
                if( updated_layer->CreateFeature( feature.get () ) != OGRERR_NONE )
                {
                    std::cerr << "Stop: Failed to create feature in layer " << DEFAULT_UPDATED_LAYER_NAME << std::endl;
                    return;
                }
            }

            for ( auto feature : retained )
            {
                if( updated_layer->CreateFeature( feature.get () ) != OGRERR_NONE )
                {
                    std::cerr << "Stop: Failed to create feature in layer " << DEFAULT_UPDATED_LAYER_NAME << std::endl;
                    return;
                }
            }
        }
    }
}
