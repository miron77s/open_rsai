#pragma once

#include "common/string_utils.h"
#include <gdal_utils/shared_dataset.h>
#include <gdal_utils/shared_feature.h>
#include <gdal_utils/shared_geometry.h>
#include "eigen_utils/geometry.h"
#include "common/definitions.h"
#include "opencv_utils/raster_roi.h"

namespace rsai
{

    namespace markup
    {

        class tile
        {
        public:
            tile ( gdal::shared_dataset raster, const gdal::bbox &bbox, int index, const std::string &source_name );

            bool                populate ( gdal::shared_dataset vector, const strings &layers );
            bool                populate ( gdal::ogr_layer * layer );

            gdal::polygon       world_border () const;
            gdal::bbox          world_bbox () const;

            cv::Mat             image () const;
            cv::Mat             render ();

            bool                populated () const;
            int                 population_size () const;

            int                 index () const;
            const std::string & source () const;

            const std::map < std::string, gdal::geometries > &  population () const;

        private:
            opencv::dataset_roi_extractor               m_extractor;
            mutable gdal::bbox                          m_bbox;
            std::map < std::string, gdal::geometries >  m_population;
            const int                                   m_index = -1;
            const std::string                           m_source_name;
        }; // class tile

        using tiles = std::list < tile >;
        using tiles_array = std::vector < tile >;

        class tile_generator
        {
        public:
            tile_generator ( gdal::shared_dataset raster, const std::string &source_name, gdal::shared_dataset denied = {} );

            tiles operator () ( const Eigen::Vector2i &tile_sizes, const double overlap ) const;
        private:
            gdal::shared_dataset m_raster;
            gdal::shared_dataset m_denied;
            const std::string    m_source_name;

        }; // class tile_generator

        class tile_balancer
        {
        public:
            tile_balancer ( const tiles &tls );

            tiles operator () ( const double populated_balance ) const;
        private:
            const tiles &m_tiles;
        };

        class tile_splitter
        {
        public:
            tile_splitter ( const tiles &tls, const double validation );

            tiles train () const;
            tiles validation () const;
        private:
            tiles m_train;
            tiles m_validation;
        };

    } // namespace markup

} // namespace rsai
