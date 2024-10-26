#pragma once

#include <vector>
#include <Eigen/Dense>
#include <opencv2/opencv.hpp>
#include "gdal_utils/shared_geometry.h"
#include "eigen_utils/geometry.h"

namespace rsai
{
    namespace building_models
    {
        class abstract
        {
        public:
            using estimates = std::vector < double >;

            abstract () = default;
            abstract ( const OGRLinearRing *object, const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step );
            abstract ( gdal::polygon object, const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step );
            abstract ( const abstract &src );
            //abstract ( abstract &&src ) = default;

            virtual gdal::multipolygons generate ( gdal::polygon object, const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step, const int vector_len ) = 0;
            virtual void                generate ( const int vector_len ) = 0;
            virtual gdal::multipolygons get      () const = 0;
            virtual void                transform_2_raster ( const Eigen::Matrix3d &transform, const Eigen::Vector2d &tile_shift ) = 0;
            virtual void                transform_2_world ( const Eigen::Matrix3d &transform, const Eigen::Vector2d &tile_shift ) = 0;
            virtual estimates           estimate ( const cv::Mat &tile, const double segmentize_step, double &memory_factor
                                               , const std::vector < cv::Mat > &segments = {}, const double proj_sigma = 3.0
                                               , const double shade_sigma = 3.0 ) const = 0;

            virtual estimates           estimate ( gdal::multipolygons structure, const Eigen::Matrix3d &world_2_raster, const Eigen::Vector2d &shift
                                               , const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step
                                               , const cv::Mat &tile, const double segmentize_step, double &memory_factor
                                               , const std::vector < cv::Mat > &segments = {}, const double proj_sigma = 3.0
                                               , const double shade_sigma = 3.0 ) const = 0;

            abstract &                  operator = ( const abstract &src );

            Eigen::Vector2d             projection_step () const;
            Eigen::Vector2d             shade_step () const;
        protected:
            gdal::polygon m_object;
            Eigen::Vector2d   m_proj_step;
            Eigen::Vector2d   m_shade_step;
        };

        class precalculated
        {
        public:
            template < class ModelType >
            precalculated ( const OGRLinearRing *object, const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step, const int from, const int to, ModelType model );

            template < class ModelType >
            precalculated ( gdal::polygon object, const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step, const int from, const int to, ModelType model );

            gdal::multipolygons         get ( const int length ) const;

            virtual abstract::estimates estimate ( const int length, const Eigen::Vector2d &shift
                                               , const cv::Mat &tile, const double segmentize_step, double &memory_factor
                                               , const std::vector < cv::Mat > &segments = {}) const;

        private:
            using items = std::vector < gdal::multipolygons >;

            std::shared_ptr < abstract > m_model;
            items m_items;
            const int m_base_index;
        };

        using precalculated_models = std::vector < std::shared_ptr < precalculated > >;

        class multiview
        {
        public:
            template < class ModelType >
            multiview ( const std::vector < gdal::polygon > &roofs, const std::vector < Eigen::Vector2d > &proj_steps, const std::vector < Eigen::Vector2d > &shade_steps
                        , const std::vector < Eigen::Matrix3d > &world_2_rasters, const int from, const int to, ModelType model );

            double estimate ( const int length, const std::vector < Eigen::Vector2d > &tile_shifts
                                           , const std::vector < cv::Mat > &tiles, const double segmentize_step, double &memory_factor
                                           , const std::vector < cv::Mat > &segments = {} ) const;

            precalculated_models models () const;
            std::vector < gdal::multipolygons > get ( const int length ) const;


            const int           range_from () const;
            const int           range_to () const;

        private:
            precalculated_models m_models;
            const int m_from;
            const int m_to;
        };

    }; // namespace building_models

}; // namespace rsai

template < class ModelType >
rsai::building_models::precalculated::precalculated ( const OGRLinearRing *object, const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step
                                                      , const int from, const int to, ModelType model )
    : m_model (new ModelType ( object, proj_step, shade_step ) ), m_base_index ( from )
{
    m_items.reserve ( to - from + 1 );
    for ( int l = from; l <= to; ++l )
    {
        m_model->generate ( l );
        m_items.push_back ( m_model->get() );
    }
}

template < class ModelType >
rsai::building_models::precalculated::precalculated ( gdal::polygon object, const Eigen::Vector2d &proj_step, const Eigen::Vector2d &shade_step
                                                      , const int from, const int to, ModelType model )
    : m_model (new ModelType ( object, proj_step, shade_step ) ), m_base_index ( from )
{
    m_items.reserve ( to - from + 1 );
    for ( int l = from; l <= to; ++l )
    {
        m_model->generate ( l );
        m_items.push_back ( m_model->get() );
    }
}

template < class ModelType >
rsai::building_models::multiview::multiview ( const std::vector < gdal::polygon > &roofs, const std::vector < Eigen::Vector2d > &proj_steps
                                              , const std::vector < Eigen::Vector2d > &shade_steps
                                              , const std::vector < Eigen::Matrix3d > &world_2_rasters, const int from, const int to, ModelType model )
    : m_from ( from ), m_to ( to )
{
    if ( proj_steps.size () == shade_steps.size() && proj_steps.size () == world_2_rasters.size () )
    {
        // Transform roofs to raster
        gdal::polygons raster_roofs;
        for ( int i = 0; i < world_2_rasters.size(); ++i )
        {
            auto roof = roofs [i];
            if ( !roof )
                raster_roofs.push_back ( nullptr );
            else
            {
                const auto &w2r = world_2_rasters [i];
                auto raster_object = gdal::operator * ( roof, w2r );
                raster_roofs.push_back ( raster_object );
            }
        }

        // Transform projection and shade vectors to raster coordinates
        std::vector < Eigen::Vector2d > proj_pixel_steps;
        std::vector < Eigen::Vector2d > shade_pixel_steps;
        for ( int i = 0; i < world_2_rasters.size(); ++i )
        {
            const auto scale_rotate = world_2_rasters [i].block < 2, 2 > ( 0, 0 );

            const auto &proj_step = proj_steps [i];
            proj_pixel_steps.emplace_back ( scale_rotate * proj_step );

            const auto &shade_step = shade_steps [i];
            shade_pixel_steps.emplace_back ( scale_rotate * shade_step );
        }

        // Calculate overall L1 norm for all vectors to retain scale
        double covering_l1_norm = 0.0;
        for ( const auto &proj_pixel_step : proj_pixel_steps )
            covering_l1_norm = std::max ( covering_l1_norm, std::max ( std::abs ( proj_pixel_step.x () ), std::abs ( proj_pixel_step.y () ) ) );

        for ( const auto &shade_pixel_step : shade_pixel_steps )
            covering_l1_norm = std::max ( covering_l1_norm, std::max ( std::abs ( shade_pixel_step.x () ), std::abs ( shade_pixel_step.y () ) ) );

        // Normalize vectors
        //std::cout << "projes ";
        for ( auto &proj_pixel_step : proj_pixel_steps )
        {
            proj_pixel_step /= covering_l1_norm;
            //std::cout << proj_pixel_step.transpose() << ' ';
        }
        //std::cout << '\n';

        //std::cout << "shades ";
        for ( auto &shade_pixel_step : shade_pixel_steps )
        {
            shade_pixel_step /= covering_l1_norm;
            //std::cout << shade_pixel_step.transpose() << ' ';
        }
        //std::cout << '\n';

        // Precalculate building models
        for ( int i = 0; i < raster_roofs.size(); ++i )
        {
            auto raster_roof = raster_roofs [i];

            if ( !raster_roof )
                m_models.push_back ( nullptr );
            else
                m_models.emplace_back ( new precalculated ( raster_roof, proj_pixel_steps [i], shade_pixel_steps [i], from, to, model ) );
        }
    }
}
