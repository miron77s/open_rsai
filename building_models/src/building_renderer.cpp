#include "rsai/building_models/building_renderer.h"

using namespace rsai;

rsai::building_renderer::building_renderer ( const cv::Mat &image, const int line_thickness ):
    m_image ( image.clone() ), m_line_thickness ( line_thickness )
{
    m_render = cv::Mat::zeros( m_image.size (), m_image.type() );
}

void rsai::building_renderer::render_roof ( const gdal::polygon &roof )
{
    render_polygon ( roof.get(), { 0, 0, 0xFF, 0x80 }, m_line_thickness );
}
void rsai::building_renderer::render_roof ( const gdal::multipolygon &roof )
{
    render_multipolygon ( roof, { 0, 0, 0xFF, 0x80 }, m_line_thickness );
}
void rsai::building_renderer::render_projection ( const gdal::multipolygon &roof )
{
    render_multipolygon ( roof, { 0, 0xFF, 0, 0x80 }, m_line_thickness );
}
void rsai::building_renderer::render_shade ( const gdal::multipolygon &roof )
{
    render_multipolygon ( roof, { 0xFF, 0, 0, 0x80 }, m_line_thickness );
}

void rsai::building_renderer::render_position ( const gdal::polygon &position, const cv::Scalar color, bool render_external, bool render_internals )
{
    render_contour ( position.get (), color, m_line_thickness, render_external, render_internals );
}

void rsai::building_renderer::render_position ( OGRPolygon *position, const cv::Scalar color, bool render_external, bool render_internals )
{
    render_contour ( position, color, m_line_thickness, render_external, render_internals );
}

void rsai::building_renderer::render_position ( const gdal::multipolygon &position, const cv::Scalar color, bool render_external, bool render_internals )
{
    render_multicontour ( position.get (), color, m_line_thickness, render_external, render_internals );
}

void rsai::building_renderer::render_position ( OGRMultiPolygon *position, const cv::Scalar color, bool render_external, bool render_internals )
{
    render_multicontour ( position, color, m_line_thickness, render_external, render_internals );
}

bool rsai::building_renderer::save ( const std::string &file_name ) const
{
    return cv::imwrite ( file_name, get () );
}

cv::Mat rsai::building_renderer::get () const
{
    const double alpha = 0.5, betta = 1.0 - alpha;

    cv::Mat mask;
    cv::cvtColor(m_render, mask, cv::COLOR_BGR2GRAY);
    cv::threshold(mask, mask, 0, 255, cv::THRESH_BINARY);

    cv::Mat out = m_image.clone();
    for (int y = 0; y < out.rows; ++y)
    {
        for (int x = 0; x < out.cols; ++x)
        {
            if ( mask.at<uchar>(y, x) > 0)
                out.at<cv::Vec3b>(y, x) = alpha * out.at<cv::Vec3b>(y, x) + betta * m_render.at<cv::Vec3b>(y, x);
        }
    }

    return out;
}

void rsai::building_renderer::render_contour ( OGRPolygon *polygon, const cv::Scalar color, const int thickness, bool render_external, bool render_internals )
{
    OGRLinearRing *exteriorRing = polygon->getExteriorRing();
    if (exteriorRing != nullptr && render_external)
    {
        std::vector<cv::Point> contour;
        for (auto j = 0; j < exteriorRing->getNumPoints(); ++j)
        {
            OGRPoint point;
            exteriorRing->getPoint(j, &point);
            contour.emplace_back(cv::Point(std::round<int>(point.getX()), std::round<int>(point.getY())));
        }
        cv::polylines(m_render, contour, true, color, thickness);
    }

    for (auto j = 0; j < polygon->getNumInteriorRings() && render_internals; ++j)
    {
        OGRLinearRing *interiorRing = polygon->getInteriorRing(j);
        if (interiorRing != nullptr)
        {
            std::vector<cv::Point> contour;
            for (auto k = 0; k < interiorRing->getNumPoints(); ++k)
            {
                OGRPoint point;
                interiorRing->getPoint(k, &point);
                contour.emplace_back(cv::Point(std::round<int>(point.getX()), std::round<int>(point.getY())));
            }
            cv::polylines(m_render, contour, true, color, thickness);
        }
    }
}

void rsai::building_renderer::render_multicontour ( OGRMultiPolygon * multipoly, const cv::Scalar color, const int thikness
                                                    , bool render_external, bool render_internals )
{
    for(int i=0; i < multipoly->getNumGeometries(); i++)
    {
        OGRGeometry *geo = multipoly->getGeometryRef(i);
        if (geo != nullptr && wkbFlatten(geo->getGeometryType()) == wkbPolygon)
        {
            OGRPolygon* polygon = geo->toPolygon();
            render_contour ( polygon, color, thikness, render_external, render_internals );
        }
    }
}

void rsai::building_renderer::render_polygon ( OGRPolygon *polygon, const cv::Scalar color, const int thickness )
{
    if (polygon != nullptr)
    {
        OGRLinearRing* exterior_ring = polygon->getExteriorRing();
        if (exterior_ring != nullptr)
        {
            cv::Point points[1][exterior_ring->getNumPoints()];
            for(int j=0; j<exterior_ring->getNumPoints(); j++)
            {
                points[0][j] = cv::Point(exterior_ring->getX(j), exterior_ring->getY(j));
            }
            const cv::Point* ppt[1] = { points[0] };
            int npt[] = { exterior_ring->getNumPoints() };
            fillPoly(m_render, ppt, npt, 1, color, cv::LINE_AA);
        }
    }
}

void rsai::building_renderer::render_multipolygon ( const gdal::multipolygon &multipoly, const cv::Scalar color, const int thikness )
{
    for(int i=0; i < multipoly->getNumGeometries(); i++)
    {
        OGRGeometry *geo = multipoly->getGeometryRef(i);
        if (geo != nullptr && wkbFlatten(geo->getGeometryType()) == wkbPolygon)
        {
            OGRPolygon* polygon = geo->toPolygon();
            render_polygon ( polygon, color, thikness );
        }
    }
}
