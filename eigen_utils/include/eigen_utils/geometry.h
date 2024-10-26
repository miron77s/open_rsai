#pragma once

#include <list>
#include <Eigen/Dense>

#include "gdal_utils/shared_geometry.h"
#include "gdal_utils/shared_dataset.h"

namespace gdal
{
    class bbox
    {
    public:
        bbox ();
        bbox ( const Eigen::Vector2d &tl, const Eigen::Vector2d &br );
        bbox ( const polygon &polygon );
        bbox ( const OGRPolygon *polygon );
        bbox ( const geometry &geometry );
        bbox ( const OGRGeometry *geometry );
        bbox ( const std::list < bbox > &bboxes );

        polygon  transform  ( const Eigen::Matrix3d &transform ) const;
        polygon  transform  ( const Eigen::Matrix2d &transform ) const;
        void            bufferize  ( const Eigen::Vector2d &buffer_sizes );

        polygon  to_polygon () const;

        bool            verify ( gdal::shared_dataset raster );
        bool            empty () const;
        void            set ( const Eigen::Vector2d &tl, const Eigen::Vector2d &br );

        const Eigen::Vector2d & top_left () const;
        const Eigen::Vector2d & bottom_right () const;
        const Eigen::Vector2d   center () const;
        const Eigen::Vector2d   size () const;

        bbox & operator += ( const Eigen::Vector2d & rh );


    private:
        Eigen::Vector2d m_top_left;
        Eigen::Vector2d m_bottom_right;
    };

    template < class Out >
    Out & operator << ( Out & out, const bbox & box );

    point        operator +   ( const OGRPoint *point, const Eigen::Vector2d &shift );
    point        operator +   ( const point &point, const Eigen::Vector2d &shift );
    multipoint   operator +   ( const OGRMultiPoint *point, const Eigen::Vector2d &shift );
    multipoint   operator +   ( const multipoint &point, const Eigen::Vector2d &shift );
    polygon      operator +   ( const OGRPolygon *polygon, const Eigen::Vector2d &shift );
    polygon      operator +   ( const polygon &polygon, const Eigen::Vector2d &shift );
    multipolygon operator +   ( const OGRMultiPolygon *polygon, const Eigen::Vector2d &shift );
    multipolygon operator +   ( const multipolygon &polygon, const Eigen::Vector2d &shift );
    geometry     operator +   ( OGRGeometry *geom, const Eigen::Vector2d &shift );
    geometry     operator +   ( geometry geom, const Eigen::Vector2d &shift );

    OGRPolygon *        operator += ( OGRPolygon *polygon, const Eigen::Vector2d &shift );
    polygon&            operator += ( polygon &polygon, const Eigen::Vector2d &shift );
    polygons&           operator += ( polygons &polys, const Eigen::Vector2d &shift );

    // Scale-rotate-shift
    point        operator *   ( const OGRPoint *point, const Eigen::Matrix3d &transform );
    point        operator *   ( const point &point, const Eigen::Matrix3d &transform );
    multipoint   operator *   ( const OGRMultiPoint *point, const Eigen::Matrix3d &transform );
    multipoint   operator *   ( const multipoint &point, const Eigen::Matrix3d &transform );
    polygon      operator *   ( const OGRPolygon *polygon, const Eigen::Matrix3d &transform );
    polygon      operator *   ( const polygon &polygon, const Eigen::Matrix3d &transform );
    multipolygon operator *   ( const OGRMultiPolygon *polygon, const Eigen::Matrix3d &transform );
    multipolygon operator *   ( const multipolygon &polygon, const Eigen::Matrix3d &transform );
    geometry     operator *   ( OGRGeometry *geom, const Eigen::Matrix3d &transform );

    OGRPolygon *        operator *=  ( OGRPolygon *polygon, const Eigen::Matrix3d &transform );
    polygon &           operator *=  ( polygon &polygon, const Eigen::Matrix3d &transform );
    polygons&           operator *=  ( polygons &polys, const Eigen::Matrix3d &transform );
    OGRMultiPolygon *   operator *=  ( OGRMultiPolygon *polygon, const Eigen::Matrix3d &transform );

    // Scale-rotate
    polygon      operator *   ( const OGRPolygon *polygon, const Eigen::Matrix2d &transform );
    polygon      operator *   ( const polygon &polygon, const Eigen::Matrix2d &transform );
    multipolygon operator *   ( const OGRMultiPolygon *polygon, const Eigen::Matrix2d &transform );
    multipolygon operator *   ( const multipolygon &polygon, const Eigen::Matrix2d &transform );

    polygon      project     ( const polygon &base, const Eigen::Vector2d &step, const int vector_len );

    polygon      bbox_and_projections_2_polygon ( const OGRPolygon *polygon, const std::list < Eigen::Vector2d > &shifts, const Eigen::Vector2d &raster_sizes
                                                   , const Eigen::Matrix3d &raster_2_world, const double buffer_size, const bool save_projections = false );

}; // namespace gdal

template < class Out >
Out & gdal::operator << ( Out & out, const bbox & box )
{
    out << '[' << box.top_left().transpose() << "],[" << box.bottom_right().transpose() << ']';
    return out;
}
