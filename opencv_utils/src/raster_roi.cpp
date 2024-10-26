#include "opencv_utils/raster_roi.h"
#include "eigen_utils/gdal_bridges.h"
#include "eigen_utils/geometry.h"

#include <vector>

using namespace opencv;

dataset_roi_extractor::dataset_roi_extractor(gdal::shared_dataset dataset) : m_dataset(dataset)
{
    __set_transforms ();
}

dataset_roi_extractor::dataset_roi_extractor( const dataset_roi_extractor & src)
    : m_dataset ( src.m_dataset )
{
    __set_transforms ();
}

dataset_roi_extractor::dataset_roi_extractor( dataset_roi_extractor && src)
    : m_dataset ( src.m_dataset )
{
    __set_transforms ();
    src.m_dataset = nullptr;
}

gdal::bbox dataset_roi_extractor::raster_bbox (gdal::polygon roi)
{
    auto raster_bounds = gdal::operator * ( roi, m_world_2_raster );

    return gdal::bbox ( raster_bounds );
}

cv::Mat dataset_roi_extractor::roi (gdal::bbox &bbox) const
{
    int topLeftX = bbox.top_left().x();
    int topLeftY = bbox.top_left().y();
    int bottomRightX = bbox.bottom_right().x();
    int bottomRightY = bbox.bottom_right().y();

    const int raster_width = m_dataset->GetRasterBand(1)->GetXSize();
    const int raster_height = m_dataset->GetRasterBand(1)->GetYSize();

    topLeftX = topLeftX < 0 ? 0 : topLeftX;
    topLeftY = topLeftY < 0 ? 0 : topLeftY;
    bottomRightX = bottomRightX >= raster_width ? raster_width - 1 : bottomRightX;
    bottomRightY = bottomRightY >= raster_height ? raster_height - 1 : bottomRightY;

    bbox.set ( {topLeftX, topLeftY}, {bottomRightX, bottomRightY} );

    const int width = bottomRightX - topLeftX;
    const int height = bottomRightY - topLeftY;

    std::vector < cv::Mat > channels { cv::Mat (height, width, CV_8UC1), cv::Mat (height, width, CV_8UC1), cv::Mat (height, width, CV_8UC1) };

    for(int bandIdx = 0; bandIdx < m_dataset->GetRasterCount(); bandIdx++)
    {
        std::lock_guard < std::mutex > lock ( m_mutex );
        GDALRasterBand* band = m_dataset->GetRasterBand(bandIdx + 1); // 1-based index
        auto result = band->RasterIO(GF_Read, topLeftX, topLeftY, width, height, channels [m_dataset->GetRasterCount() - bandIdx - 1].ptr (), width, height, GDT_Byte, 0, 0);
    }

    cv::Mat frame;
    cv::merge(channels, frame);

    return frame;
}

Eigen::Matrix3d dataset_roi_extractor::raster_2_world () const
{
    return m_raster_2_world;
}

Eigen::Matrix3d dataset_roi_extractor::world_2_raster () const
{
    return m_world_2_raster;
}

void dataset_roi_extractor::__set_transforms ()
{
    eigen::gdal_dataset_bridge ds_adaper ( m_dataset );
    m_raster_2_world = ds_adaper.transform();
    m_world_2_raster = m_raster_2_world.inverse();

    std::cout.flush ();
}
