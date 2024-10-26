#pragma once

#include <opencv2/opencv.hpp>
#include "gdal_utils/shared_geometry.h"
#include "differentiation/convolution_mask.h"

namespace opencv
{
    class blender_max
    {
    public:
        template < class Data_Type >
        Data_Type operator () ( Data_Type op1, Data_Type op2 ) const;
    };

    class geometry_renderer
    {
    public:
        geometry_renderer ( const cv::Size size ): m_image ( cv::Mat::zeros ( size, CV_32S ) ) {}

        template < class Blending_Function >
        bool operator () ( const gdal::polygon &poly, const convolution_mask &render_mask, Blending_Function blender );

        template < class Blending_Function >
        bool operator () ( cv::Mat mask, Blending_Function blender );

        cv::Mat & image () { return m_image; }

    private:
        cv::Mat m_image;

        template < class Blending_Function >
        bool __render ( const gdal::ring &ring, const convolution_mask &render_mask, Blending_Function blender );
    };

    template < class Blending_Function >
    bool geometry_renderer::operator () ( const gdal::polygon &poly, const convolution_mask &render_mask, Blending_Function blender )
    {
        if (poly == nullptr)
            return false;

        bool result = true;

        gdal::ring exterior_ring ( poly->getExteriorRing()->clone() );
        result = __render ( exterior_ring, render_mask, blender );

        int numInteriorRings = poly->getNumInteriorRings();
        for (int i = 0; i < numInteriorRings; i++)
        {
            gdal::ring interior_ring ( poly->getInteriorRing(i)->clone () );
            result = result && __render ( interior_ring, render_mask, blender );
        }

        return result;
    }

    template < class Blending_Function >
    bool  geometry_renderer::__render ( const gdal::ring &ring, const convolution_mask &render_mask, Blending_Function blender )
    {
        if (ring == nullptr)
            return false;

        ring->segmentize ( 1.0 );
        for (int i = 0; i < ring->getNumPoints(); i++)
        {
            const auto x = ring->getX(i);
            const auto y = ring->getY(i);

            for ( auto & item : render_mask )
            {
                const auto px = x + item.at.x,
                           py = y + item.at.y;
                if ( px >= 0 && px < m_image.cols && py >= 0 && py < m_image.rows )
                {
                    auto &mat_value = m_image.at < int32_t > ( py, px );
                    mat_value = blender ( item.value, mat_value );
                }
            }
        }

        return true;
    }

    template < class Blending_Function >
    bool geometry_renderer::operator () ( cv::Mat mask, Blending_Function blender )
    {
        if (m_image.size() != mask.size() || m_image.type() != mask.type())
        {
            return false;
        }

        if (m_image.type() != CV_32S)
        {
            return false;
        }

        for (int y = 0; y < m_image.rows; ++y) {
            for (int x = 0; x < m_image.cols; ++x)
            {
                auto &out_value = m_image.at<int>(y, x);
                out_value = blender(out_value, mask.at<int32_t>(y, x));
            }
        }

        return true;
    }

    template < class Data_Type >
    Data_Type blender_max::operator () ( Data_Type op1, Data_Type op2 ) const
    {
        return std::max ( op1, op2 );
    }


}
