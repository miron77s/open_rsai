#pragma once

#include <opencv2/opencv.hpp>

namespace opencv
{
    cv::Mat compute_gradient_magnitude(const cv::Mat& input_image, int kernel_size = 3);
};
