#include "differentiation/edge_detection.h"

#include <stdexcept>

using namespace cv;

cv::Mat opencv::compute_gradient_magnitude(const cv::Mat& input_image, int kernel_size)
{
    // Ensure kernel size is odd
    if (kernel_size % 2 == 0)
    {
        throw std::invalid_argument("Kernel size must be odd");
    }

    cv::Mat gradX, gradY;
    cv::Mat absGradX, absGradY;
    cv::Mat grad;

    // Compute gradients in X and Y directions
    cv::Sobel(input_image, gradX, CV_32F, 1, 0, kernel_size, 1, 0, BORDER_DEFAULT);
    cv::Sobel(input_image, gradY, CV_32F, 0, 1, kernel_size, 1, 0, BORDER_DEFAULT);

    // Compute absolute gradients
    cv::convertScaleAbs(gradX, absGradX);
    cv::convertScaleAbs(gradY, absGradY);

    // Compute total gradient (approximate)
    cv::addWeighted(absGradX, 0.5, absGradY, 0.5, 0, grad);

    return grad;
}
