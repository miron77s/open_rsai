#include "eigen_utils/gdal_bridges.h"

#include <ogrsf_frmts.h>

eigen::gdal_dataset_bridge::gdal_dataset_bridge ( const gdal::shared_dataset &ds )
    : _ds ( ds )
{
    __set_transform ();
    __set_geo_bounds ();
}

Eigen::Matrix3d eigen::gdal_dataset_bridge::transform () const
{
    return _transform;
}

Eigen::Matrix2d eigen::gdal_dataset_bridge::geo_bounds () const
{
    return _geo_bounds;
}

gdal::polygon eigen::gdal_dataset_bridge::geo_bounds_polygon () const
{
    OGRLinearRing * ring = new OGRLinearRing;
    ring->addPoint ( _geo_bounds ( 0, 0 ), _geo_bounds ( 0, 1 ) );
    ring->addPoint ( _geo_bounds ( 1, 0 ), _geo_bounds ( 0, 1 ) );
    ring->addPoint ( _geo_bounds ( 1, 0 ), _geo_bounds ( 1, 1 ) );
    ring->addPoint ( _geo_bounds ( 0, 0 ), _geo_bounds ( 1, 1 ) );

    gdal::polygon bounds ( new OGRPolygon );
    bounds->addRingDirectly ( ring );
    bounds->closeRings ();

    return bounds;
}

void eigen::gdal_dataset_bridge::apply_geo_bounds_to_layer ( OGRLayer * layer ) const
{
    if ( layer )
        layer->SetSpatialFilterRect (
                                        std::min ( _geo_bounds ( 0, 0 ), _geo_bounds ( 1, 0 ) )
                                      , std::min ( _geo_bounds ( 0, 1 ), _geo_bounds ( 1, 1 ) )
                                      , std::max ( _geo_bounds ( 0, 0 ), _geo_bounds ( 1, 0 ) )
                                      , std::max ( _geo_bounds ( 0, 1 ), _geo_bounds ( 1, 1 ) )
                                    );
}

void eigen::gdal_dataset_bridge::__set_transform ()
{
    if ( _ds )
    {
        double t [6];
        _ds->GetGeoTransform ( t );

        Eigen::Matrix3d result;
        _transform << t [1], t [2], t [0],
                      t [4], t [5], t [3],
                      0,     0,     1;
    }
    else
        _transform = Eigen::Matrix3d::Identity ();
}

void eigen::gdal_dataset_bridge::__set_geo_bounds ()
{
    if ( _ds )
    {
        const int w = _ds->GetRasterXSize () - 1,
                  h = _ds->GetRasterYSize () - 1;

        Eigen::Vector3d tl ( 0, 0, 1 ),
                        br ( w, h, 1 );

        auto tr = transform ();

        Eigen::Vector3d geo_tl = tr * tl,
                        geo_br = tr * br;

        _geo_bounds << geo_tl [0], geo_tl [1]
                     , geo_br [0], geo_br [1];
    }
}
