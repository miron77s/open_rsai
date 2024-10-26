#pragma once

#include "rsai/objects_bounds_finder.h"

#include <ogrsf_frmts.h>

#include "common/definitions.h"
#include "gdal_utils/all_helpers.h"
#include "eigen_utils/vector_helpers.h"
#include "eigen_utils/gdal_bridges.h"
#include "eigen_utils/geometry.h"
#include "gdal_utils/shared_options.h"
#include "gdal_utils/shared_feature.h"

template < class PromtFunc, class ProgressFunc >
rsai::objects_bounds_finder::objects_bounds_finder (
                                                     gdal::shared_dataset &ds_vector
                                                   , gdal::shared_dataset &ds_raster
                                                   , gdal::shared_dataset &ds_meta
                                                   , gdal::shared_dataset &ds_out
                                                   , const int mask_size
                                                   , const int max_projection_length
                                                   , const bool force_rewrite
                                                   , const PromtFunc &promt_func
                                                   , const ProgressFunc &progress_func
                                             )
{
    Eigen::Vector2d proj_vector, shade_vector;

    // Getting metadata
    auto meta_layer = ds_meta->GetLayer ( 0 );
    if ( meta_layer )
    {
        // Find projection vector
        const std::string proj_filter = std::string ( DEFAULT_META_TYPE_FIELD_NAME ) + " = '" + DEFAULT_PROJECTION_FIELD_VALUE + "'";
        if( meta_layer->SetAttributeFilter( proj_filter.c_str() ) != OGRERR_NONE )
        {
            std::cerr << "STOP: Error setting metadata attribute filter " << proj_filter << '\n';
            return;
        }
        else
        {
            meta_layer->ResetReading();
            gdal::shared_feature feature = meta_layer->GetNextFeature();
            if ( feature )
            {
                auto geometry = feature->GetGeometryRef();
                if ( wkbFlatten( geometry->getGeometryType() ) == wkbLineString)
                {
                    auto vector = geometry->toLineString();
                    const int points = vector->getNumPoints();
                    if ( points > 1 )
                    {
                        OGRPoint p1, p2;
                        vector->getPoint ( 0, &p1 );
                        vector->getPoint ( 1, &p2 );

                        proj_vector = Eigen::Vector2d ( p2.getX () - p1.getX (), p2.getY() - p1.getY () );
                    }
                }
            }
        }

        // Find shade vector
        const std::string shade_filter = std::string ( DEFAULT_META_TYPE_FIELD_NAME ) + " = '" + DEFAULT_SHADE_FIELD_VALUE + "'";
        if( meta_layer->SetAttributeFilter( shade_filter.c_str() ) != OGRERR_NONE )
        {
            std::cerr << "STOP: Error setting metadata attribute filter " << shade_filter << '\n';
            return;
        }
        else
        {
            meta_layer->ResetReading();
            gdal::shared_feature feature = meta_layer->GetNextFeature();
            if ( feature )
            {
                auto geometry = feature->GetGeometryRef();
                if ( wkbFlatten( geometry->getGeometryType() ) == wkbLineString)
                {
                    auto vector = geometry->toLineString();
                    const int points = vector->getNumPoints();
                    if ( points > 1 )
                    {
                        OGRPoint p1, p2;
                        vector->getPoint ( 0, &p1 );
                        vector->getPoint ( 1, &p2 );

                        shade_vector = Eigen::Vector2d ( p2.getX () - p1.getX (), p2.getY() - p1.getY () );
                    }
                }
            }
        }
    }

    std::cout << "projection " << proj_vector.transpose() << '\n';
    std::cout << "shade " << shade_vector.transpose() << '\n';

    // Obtain raster-2-world and vice versa transforms
    eigen::gdal_dataset_bridge raster_eigen ( ds_raster );
    Eigen::Matrix3d raster_2_world = raster_eigen.transform ();
    Eigen::Matrix3d world_2_raster = raster_2_world.inverse ();

    const double L1_norm = std::fmax (
                                        std::fmax ( std::fabs ( proj_vector [0] ),  std::fabs ( proj_vector [1] ) )
                                      , std::fmax ( std::fabs ( shade_vector [0] ), std::fabs ( shade_vector [1] ) )
                                     );
    const Eigen::Vector2d proj_step  = proj_vector / L1_norm;
    const Eigen::Vector2d shade_step = shade_vector / L1_norm;

    // Projecting and shading vectors of maximum length
    const Eigen::Vector2d proj_max   = proj_step * max_projection_length;
    const Eigen::Vector2d shade_max  = shade_step * max_projection_length;

    // Source object together with projecting and shading
    std::list < Eigen::Vector2d > shifts = { { 0, 0 }, proj_max, shade_max };

    const int layers = ds_vector->GetLayerCount ();

    for( int i = 0; i < layers; ++i )
    {
        auto layer = ds_vector->GetLayer ( i );
        const std::string layer_name = layer->GetName();

        gdal::create_vector_helper layers_helper ( ds_out, std::cerr );
        auto out_layer = layers_helper.create_layer ( DEFAULT_BOUNDS_LAYER_NAME, layer->GetGeomType (), layer->GetSpatialRef(), force_rewrite, promt_func );
        if ( !out_layer )
            continue;

        // Creating required further fields
        out_layer << gdal::field_definition ( DEFAULT_OBJECT_ID_FIELD_NAME,     OFTInteger )
                  << gdal::field_definition ( DEFAULT_PROJ_STEP_X_FIELD_NAME,   OFTReal )
                  << gdal::field_definition ( DEFAULT_PROJ_STEP_Y_FIELD_NAME,   OFTReal )
                  << gdal::field_definition ( DEFAULT_SHADE_STEP_X_FIELD_NAME,  OFTReal )
                  << gdal::field_definition ( DEFAULT_SHADE_STEP_Y_FIELD_NAME,  OFTReal )
                  << gdal::field_definition ( DEFAULT_VECTOR_MAX_LENGTH_NAME,   OFTInteger );

        // Creating feature with fixed fields' values
        gdal::shared_feature new_feature ( out_layer->GetLayerDefn() );
        new_feature->SetField ( DEFAULT_OBJECT_ID_FIELD_NAME,    0 );
        new_feature->SetField ( DEFAULT_PROJ_STEP_X_FIELD_NAME,  proj_vector [0] );
        new_feature->SetField ( DEFAULT_PROJ_STEP_Y_FIELD_NAME,  proj_vector [1] );
        new_feature->SetField ( DEFAULT_SHADE_STEP_X_FIELD_NAME, shade_vector [0] );
        new_feature->SetField ( DEFAULT_SHADE_STEP_Y_FIELD_NAME, shade_vector [1] );
        new_feature->SetField ( DEFAULT_VECTOR_MAX_LENGTH_NAME,  max_projection_length );

        const float features_count = layer->GetFeatureCount ();
        int features_processed = 0;

        for ( const auto& feature: *layer )
        {
            progress_func ( i + 1, layers, ++features_processed / features_count );

            const OGRGeometry *geometry = feature->GetGeometryRef ();
            if ( geometry != nullptr
                    && wkbFlatten ( geometry->getGeometryType()) == wkbPolygon )
            {
                auto polygon = geometry->toPolygon();

                gdal::polygon simplified_geometry ( new polygon::element_type );
                simplified_geometry->addRing ( const_cast < OGRLinearRing * > ( polygon->getExteriorRing () ) );

                // calculating bbox for RasterIO
                auto bbox_geometry = gdal::bbox_and_projections_2_polygon ( simplified_geometry.get (), shifts
                                                                            , { ds_raster->GetRasterXSize () - 1, ds_raster->GetRasterYSize () - 1 }
                                                                            , raster_2_world, mask_size );
                if ( bbox_geometry == nullptr )
                    continue;

                // Saving bbox together with polygon
                bbox_geometry << simplified_geometry;
                bbox_geometry->closeRings ();

                new_feature->SetGeometry ( bbox_geometry.get () );
                new_feature->SetField ( DEFAULT_OBJECT_ID_FIELD_NAME, features_processed );

                if( out_layer->CreateFeature( new_feature.get () ) != OGRERR_NONE )
                {
                    std::cerr << "Stop: Failed to create feature." << std::endl;
                    return;
                }
            }
        }
        progress_func ( i + 1, layers, 1.0f, true );
    }
}

