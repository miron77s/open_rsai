#include "rsai/building_models/structure_estimator.h"

#include <fstream>
#include "opencv_utils/geometry_renderer.h"
#include "differentiation/gauss_directed_derivative.h"
#include "differentiation/convolution_mask.h"

rsai::building_models::structure_estimator::structure_estimator ( prismatic model, const roof_responses &responses
                                                                , const cv::Mat &tile_gray, const Eigen::Vector2d &tile_tl_corner
                                                                , const Eigen::Matrix3d &world_2_raster, const double segmentize_step
                                                                , const gdal::polygons &segments )
    : m_model ( model ), m_responses ( responses ), m_tile_gray ( tile_gray ), m_tile_tl_corner ( tile_tl_corner )
    , m_world_2_raster ( world_2_raster ), m_segmentize_step ( segmentize_step ), m_segments ( segments )
{

}

rsai::building_models::structures rsai::building_models::structure_estimator::operator () ( const double max_length, const double projection_step
                                                                                               , const int roof_responses_max, const int shade_responses_max ) const
{
    //static std::atomic<int> model_counter(0);
    //++model_counter;
    //std::ofstream out ( "logs/" + std::to_string ( model_counter ) );

    structure_map roof_map;

    const double sigma = 3;
    const int mask_half = std::round ( sigma * 4.0 );
    convolution_mask mask ( gauss::function ( sigma ), mask_half, mask_half );

    std::vector < cv::Mat > segment_maps ( m_segments.size() );
    for ( int i = 0; i < m_segments.size(); ++i )
    {
        const auto & segment = m_segments [i];
        auto & segment_map = segment_maps [i];
        opencv::geometry_renderer renderer ( m_tile_gray.size () );
        renderer ( segment, mask, opencv::blender_max () );

        cv::Mat image = renderer.image();
        double minVal, maxVal;
        cv::minMaxLoc(image, &minVal, &maxVal);

        image.convertTo(image, CV_8U, 255.0/maxVal, 0);

        segment_maps [i] = image;
    }

    std::vector < double > memory_weights ( roof_responses_max );
    for ( int length = 1; length <= max_length; length += projection_step )
    {
        m_model.generate ( length );

        //out << "\nlength " << length << '\n';

        for ( int j = 0; j < roof_responses_max && j < m_responses.size(); ++j )
        {
            const auto &roof_shift = m_responses.at ( j ).shift_on_tile;

            auto local_model = m_model;

            local_model.transform_2_raster ( m_world_2_raster, -m_tile_tl_corner + roof_shift );
            auto estimates = local_model.estimate ( m_tile_gray, m_segmentize_step, memory_weights [j], segment_maps );

            double estamate = estimates [0] * estimates [1] * m_responses.at ( j ).value;
            //double estamate = ( estimates [0] + estimates [2] ) * estimates [1];

            //out << '\t' << estimates [0] << '\t' << estimates [1] << '\t' << estimates [2] << '\n';
            /*for ( auto e : estimates )
            {
                estamate *= e;
            }*/

            if ( roof_map.find ( roof_shift ) == roof_map.end () )
            {
                auto & roof_response = roof_map [roof_shift];
                roof_response.shift_on_tile = roof_shift;
                roof_response.shades.reserve ( max_length );
                roof_response.shades.emplace_back ( length, estamate, local_model );
                roof_response.value = std::max ( roof_response.value, estamate  );
            }
            else
            {
                auto & roof_response = roof_map [roof_shift];
                roof_response.shades.emplace_back ( length, estamate, local_model );
                roof_response.value = std::max ( roof_response.value, estamate );
            }
        }
    }

    for ( auto & roof_response_pair : roof_map )
    {
        auto & roof_response = roof_response_pair.second;
        auto & shade_responses = roof_response.shades;

        std::sort ( shade_responses.begin() + 1, shade_responses.end(), std::greater < model_response > () );

        const int num_items_to_copy = std::min(shade_responses_max, static_cast<int>(shade_responses.size()));
        decltype ( roof_response.shades ) shade_responses_limited;
        std::copy_n(shade_responses.begin(), num_items_to_copy, std::back_inserter(shade_responses_limited));

        roof_response.shades = std::move ( shade_responses_limited );
    }

    structures result;
    result.reserve ( roof_map.size() );
    for (auto &pair : roof_map)
        result.push_back( std::move ( pair.second ) );

    std::sort ( result.begin(), result.end(), std::greater < structure > () );

    return std::move ( result );
}
