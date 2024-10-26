#pragma once

#include <utility>
#include <Eigen/Dense>

#include "gdal_utils/shared_dataset.h"
#include "gdal_utils/shared_geometry.h"

namespace eigen
{
    class gdal_dataset_bridge
    {
    public:
        gdal_dataset_bridge ( const gdal::shared_dataset &ds );

        Eigen::Matrix3d         transform () const;

        Eigen::Matrix2d         geo_bounds () const;

        gdal::polygon    geo_bounds_polygon () const;

        void                    apply_geo_bounds_to_layer ( OGRLayer * layer ) const;

    private:
        gdal::shared_dataset _ds;
        Eigen::Matrix3d _transform;
        Eigen::Matrix2d _geo_bounds;

        void    __set_transform ();
        void    __set_geo_bounds ();
    };

    template < class Type >
    Eigen::Vector2 < Type > cast ( const gdal::point & pt )
    {
        return Eigen::Vector2 < Type > ( pt->getX (), pt->getY () );
    }


}; //namespace eigen
