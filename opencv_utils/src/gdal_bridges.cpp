#include "opencv_utils/gdal_bridges.h"

opencv::polygon_bounded_ops::polygon_bounded_ops(const cv::Mat &vals, const gdal::polygon poly)
    : m_values(vals), m_polygon(poly) {}

double opencv::polygon_bounded_ops::sum(double dx, double dy)
{
    double sum = 0.0;

    auto ring = m_polygon->getExteriorRing();

    OGRPoint point;
    for(int i = 0; i < ring->getNumPoints(); ++i)
    {
        ring->getPoint(i, &point);

        // shift the point
        int x = static_cast<int>(point.getX() + dx);
        int y = static_cast<int>(point.getY() + dy);

        // check if the point is within the Mat range
        if(x >= 0 && x < m_values.cols && y >= 0 && y < m_values.rows)
            sum += m_values.at<uint8_t>(y, x);
    }

    return sum;
}

double opencv::polygon_bounded_ops::sum_unsafe(double dx, double dy)
{
    double sum = 0.0;

    auto ring = m_polygon->getExteriorRing();

    uint8_t * data = m_values.data;
    auto pitch = m_values.cols;

    const auto points_count = ring->getNumPoints();
    OGRRawPoint * points = new OGRRawPoint [points_count];
    ring->getPoints ( points );

    //OGRPoint point;
    for(int i = 0; i < points_count; ++i)
    {
        //ring->getPoint(i, &point);

        // shift the point
        const int x = static_cast<int>(points [i].x + dx);
        const int y = static_cast<int>(points [i].y + dy);

        //sum += m_values.at<uint8_t>(y, x);
        sum += *( data + y * pitch + x );
    }

    delete [] points;

    return sum;
}

double opencv::polygon_bounded_ops::nonzero_weighted_sum(double dx, double dy)
{
    double sum = 0.0;
    int count = 0;

    auto ring = m_polygon->getExteriorRing();

    for(int i = 0; i < ring->getNumPoints(); ++i)
    {
        OGRPoint point;
        ring->getPoint(i, &point);

        // shift the point
        int x = static_cast<int>(point.getX() + dx);
        int y = static_cast<int>(point.getY() + dy);

        const auto value = m_values.at<uint8_t>(y, x);

        count += ( value > 0.0 ) ? 1 : 0;

        if ( x >= 0 && x < m_values.cols && y >= 0 && y < m_values.rows )
            sum += value;
    }

    return sum * count;
}
