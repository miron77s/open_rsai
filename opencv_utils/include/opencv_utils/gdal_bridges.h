#pragma once

#include <opencv2/opencv.hpp>
#include "gdal_utils/shared_geometry.h"

namespace opencv
{
    class polygon_bounded_ops
    {
    public:
        polygon_bounded_ops() = default;
        polygon_bounded_ops( const polygon_bounded_ops & ) = default;
        polygon_bounded_ops(const cv::Mat &vals, const gdal::polygon poly);

        double sum(double dx, double dy);
        double sum_unsafe(double dx, double dy);
        double nonzero_weighted_sum(double dx, double dy);

    private:
        cv::Mat m_values;
        const gdal::polygon m_polygon;
    };
}; // namespace opencv
