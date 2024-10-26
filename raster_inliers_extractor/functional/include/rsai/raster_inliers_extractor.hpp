#pragma once

#include "rsai/raster_inliers_extractor.h"

#include <ogrsf_frmts.h>

#include <gdal_utils/all_helpers.h>
#include <gdal_utils/shared_options.h>
#include <gdal_utils/region_of_interest.h>
#include "gdal_utils/shared_feature.h"

#include "common/definitions.h"

template < class PromtFunc, class ProgressFunc >
rsai::raster_inliers_extractor::raster_inliers_extractor ( gdal::shared_dataset &ds_vector
                                                           , gdal::shared_dataset &ds_raster
                                                           , gdal::shared_dataset &ds_out
                                                           , gdal::shared_dataset &ds_roi
                                                           , const bool force_rewrite
                                                           , const bool crop_geometry
                                                           , const std::string &semanic_filter
                                                           , const int min_raster_obj_size
                                                           , const PromtFunc &promt_func
                                                           , const ProgressFunc &progress_func
                                                         )
{
    // getting input raster image geo boundaries
    eigen::gdal_dataset_bridge raster_eigen ( ds_raster );
    auto raster_bounds = raster_eigen.geo_bounds_polygon ();

    Eigen::Matrix3d raster_2_world = raster_eigen.transform ();
    Eigen::Matrix3d world_2_raster = raster_2_world.inverse ();

    // getting roi if available
    gdal::region_of_interest roi_wrp ( ds_roi );
    auto roi = roi_wrp.first_found ();

    // finding intersection for raster bounds and roi
    gdal::geometry final_roi ( raster_bounds );

    if ( roi != nullptr )
        final_roi = gdal::geometry ( raster_bounds->Intersection ( roi.get () ) );

    const int layers = ds_vector->GetLayerCount ();

    for( int i = 0; i < layers; ++i )
    {
        auto layer = ds_vector->GetLayer ( i );
        const std::string layer_name = layer->GetName();

        // spatial filter by raster bounds and roi
        layer->SetSpatialFilter ( final_roi.get () );
        layer->SetAttributeFilter ( semanic_filter.c_str() );

        gdal::create_vector_helper layers_helper ( ds_out, std::cerr );
        auto out_layer = layers_helper.create_layer ( DEFAULT_INLIERS_LAYER_NAME, layer->GetGeomType ()
                                                      , layer->GetSpatialRef(), force_rewrite, promt_func );

        if ( !out_layer )
            continue;

        auto layer_defn = layer->GetLayerDefn();

        for ( int i = 0; i < layer_defn->GetFieldCount (); ++i )
            out_layer->CreateField ( layer_defn->GetFieldDefn ( i ) );

        const float features_count = layer->GetFeatureCount ();
        float features_processed = 0.0f;

        for ( const auto& feature: *layer )
        {
            progress_func ( i + 1, layers, ++features_processed / features_count );

            const OGRGeometry *geometry = feature->GetGeometryRef ();
            if ( geometry != nullptr
                    && wkbFlatten ( geometry->getGeometryType()) == wkbPolygon )
            {

                auto polygon = geometry->toPolygon();

                shared_feature new_feature ( feature->Clone () );

                // Crop geometry if required
                if ( crop_geometry )
                {
                    auto cropped_geom = polygon->Intersection( final_roi.get() );

                    OGRwkbGeometryType geom_type = cropped_geom->getGeometryType();
                    if ( wkbFlatten(geom_type) == wkbPolygon )      // Crop result is polygon
                    {
                        if ( !__check_raster_size ( cropped_geom, world_2_raster, min_raster_obj_size ) )
                            continue;

                        new_feature->SetGeometry ( cropped_geom );

                        if( out_layer->CreateFeature( new_feature.get () ) != OGRERR_NONE )
                        {
                            std::cerr << "Stop: Failed to create feature." << std::endl;
                            return;
                        }
                    }
                    else    // Crop result is multipolygon - add the output individually
                    {
                        gdal::multipolygon cropped_multi_polygon ( cropped_geom->toMultiPolygon() );

                        for(int i = 0; i < cropped_multi_polygon->getNumGeometries(); ++i)
                        {
                            auto polygon = cropped_multi_polygon->getGeometryRef(i);

                            if ( !__check_raster_size ( polygon, world_2_raster, min_raster_obj_size ) )
                                continue;

                            new_feature->SetGeometry ( polygon );

                            if( out_layer->CreateFeature( new_feature.get () ) != OGRERR_NONE )
                            {
                                std::cerr << "Stop: Failed to create feature." << std::endl;
                                return;
                            }
                        }
                    }
                }
                else // No crop - simply add polygon to the output map
                {
                    if ( !__check_raster_size ( polygon, world_2_raster, min_raster_obj_size ) )
                        continue;

                    new_feature->SetGeometry ( polygon );

                    if( out_layer->CreateFeature( new_feature.get () ) != OGRERR_NONE )
                    {
                        std::cerr << "Stop: Failed to create feature." << std::endl;
                        return;
                    }
                }
            }
        }
        progress_func ( i + 1, layers, 1.0f, true );
    }
}

template < class GeometryType >
bool rsai::raster_inliers_extractor::__check_raster_size ( std::shared_ptr < GeometryType > geometry, const Eigen::Matrix3d &world_2_raster, const int min_raster_obj_size )
{
    return __check_raster_size ( geometry.get (), world_2_raster, min_raster_obj_size );
}

template < class GeometryType >
bool rsai::raster_inliers_extractor::__check_raster_size ( GeometryType * geometry, const Eigen::Matrix3d &world_2_raster, const int min_raster_obj_size )
{
    gdal::bbox bbox ( geometry );
    auto raster_bbox_polygon = bbox.transform ( world_2_raster );
    gdal::bbox raster_bbox ( raster_bbox_polygon );
    auto raster_sizes = raster_bbox.size();

    // skip object if it is too small
    return raster_sizes.x() > min_raster_obj_size && raster_sizes.y () > min_raster_obj_size;
}
