#pragma once

#include <string>
#include <opencv2/opencv.hpp>
#include "gdal_utils/shared_geometry.h"

namespace rsai
{
    class building_renderer
    {
    public:
        building_renderer ( const cv::Mat &image, const int line_thickness = 2 );

        void render_roof ( const gdal::polygon &roof );
        void render_roof ( const gdal::multipolygon &roof );
        void render_projection ( const gdal::multipolygon &roof );
        void render_shade ( const gdal::multipolygon &roof );

        void render_position ( const gdal::polygon &position, const cv::Scalar color, bool render_external = true, bool render_internals = true );
        void render_position ( OGRPolygon *position, const cv::Scalar color, bool render_external = true, bool render_internals = true );
        void render_position ( const gdal::multipolygon &position, const cv::Scalar color, bool render_external = true, bool render_internals = true );
        void render_position ( OGRMultiPolygon *position, const cv::Scalar color, bool render_external = true, bool render_internals = true );

        bool save ( const std::string &file_name ) const;

        cv::Mat get () const;

    private:
        cv::Mat m_image;
        cv::Mat m_render;
        const int m_line_thickness;

        void render_contour ( OGRPolygon *polygon, const cv::Scalar color, const int thickness, bool render_external, bool render_internals );
        void render_multicontour ( OGRMultiPolygon * multipoly, const cv::Scalar color, const int thikness, bool render_external, bool render_internals );
        void render_polygon ( OGRPolygon *polygon, const cv::Scalar color, const int thickness );
        void render_multipolygon ( const gdal::multipolygon &multipoly, const cv::Scalar color, const int thikness );
    }; // class buiding_variants_saver
}; // namespace rsai
