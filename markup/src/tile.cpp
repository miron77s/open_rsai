#include "rsai/markup/tile.h"

#include <algorithm>
#include <random>
#include "rsai/building_models/building_renderer.h"
#include "eigen_utils/gdal_bridges.h"

using namespace rsai::markup;
using namespace gdal;

rsai::markup::tile::tile ( gdal::shared_dataset raster, const gdal::bbox &bbox, int index, const std::string &source_name )
    : m_extractor ( raster ), m_bbox ( bbox ), m_index ( index ), m_source_name ( source_name )
{
}

bool rsai::markup::tile::populate ( gdal::shared_dataset vector, const strings &layers )
{
    bool result = false;
    for ( const auto layer_name : layers )
    {
        auto layer = vector->GetLayerByName ( layer_name.c_str() );
        auto populated = populate ( layer );
        result = result || populated;
    }

    return result;
}

bool rsai::markup::tile::populate ( gdal::ogr_layer * layer )
{
    auto tile_border = world_border ();
    auto world_2_raster = m_extractor.world_2_raster();

    layer->SetSpatialFilter ( tile_border.get () );
    layer->ResetReading ();

    auto & layer_population = m_population [layer->GetName()];
    int start_population = layer_population.size();
    while ( gdal::shared_feature feature = layer->GetNextFeature() )
    {
        auto raster_geometry = feature->GetGeometryRef() * world_2_raster;
        if ( !raster_geometry )
            continue;

        auto tile_geometry = raster_geometry + -m_bbox.top_left();
        if ( !tile_geometry )
            continue;

        layer_population.push_back ( tile_geometry );
    }

    bool added_population = layer_population.size() - start_population;
    return added_population;
}

cv::Mat rsai::markup::tile::image () const
{
    return m_extractor.roi ( m_bbox );
}

cv::Mat rsai::markup::tile::render ()
{
    auto tile_image = image ();
    rsai::building_renderer render ( tile_image );

    for ( auto &layer_population : m_population )
    {
        for ( auto &geometry : layer_population.second )
        {
            if ( gdal::is_polygon ( geometry ) )
                render.render_position ( geometry->toPolygon(), cv::Scalar ( 0x00, 0xFF, 0x00 ) );
            else if ( gdal::is_multipolygon ( geometry ) )
                render.render_position ( geometry->toMultiPolygon(), cv::Scalar ( 0x00, 0xFF, 0x00 ) );

        }
    }

    return render.get();
}

bool rsai::markup::tile::populated () const
{
    for ( const auto & layer_population : m_population )
    {
        if ( layer_population.second.size() != 0 )
            return true;
    }
    return false;
}

int rsai::markup::tile::population_size () const
{
    int population = 0;
    for ( const auto & layer_population : m_population )
    {
        population += layer_population.second.size();
    }
    return population;
}

gdal::polygon rsai::markup::tile::world_border () const
{
    return m_bbox.transform ( m_extractor.raster_2_world() );
}

gdal::bbox rsai::markup::tile::world_bbox () const
{
    return gdal::bbox ( world_border () );
}

int rsai::markup::tile::index () const
{
    return m_index;
}

const std::string & rsai::markup::tile::source () const
{
    return m_source_name;
}

const std::map < std::string, gdal::geometries > & rsai::markup::tile::population () const
{
    return m_population;
}

rsai::markup::tile_generator::tile_generator ( gdal::shared_dataset raster, const std::string &source_name, gdal::shared_dataset denied )
    : m_raster ( raster ), m_source_name ( source_name ), m_denied ( denied )
{
}

tiles rsai::markup::tile_generator::operator () ( const Eigen::Vector2i &tile_sizes, const double overlap ) const
{
    const Eigen::Vector2i raster_sizes ( m_raster->GetRasterXSize(), m_raster->GetRasterYSize() );
    const Eigen::Vector2i tile_step = ( tile_sizes.cast < double > () * ( 1.0 - overlap ) ).cast < int > ();
    const Eigen::Vector2d tile_sizes_d = tile_sizes.cast < double > ();

    eigen::gdal_dataset_bridge ds_adaper ( m_raster );
    auto raster_2_world = ds_adaper.transform();

    gdal::ogr_layer * denied_layer = ( m_denied ) ? m_denied->GetLayer ( 0 ) : nullptr;

    tiles result;
    int tile_index = 0;
    for ( int y = 0; y < raster_sizes.y (); y += tile_step.y () )
    {
        for ( int x = 0; x < raster_sizes.x (); x += tile_step.x () )
        {
            ++tile_index;

            const Eigen::Vector2d tile_pos ( x, y );
            const gdal::bbox raster_bbox ( tile_pos, tile_pos + tile_sizes_d );
            const auto world_bbox_poly = raster_bbox.transform ( raster_2_world );

            if ( denied_layer )
            {
                denied_layer->SetSpatialFilter ( world_bbox_poly.get () );
                denied_layer->ResetReading();

                // Tile intersects with denied region
                if ( gdal::shared_feature feature = denied_layer->GetNextFeature() )
                    continue;
            }

            result.emplace_back ( m_raster, raster_bbox, tile_index, m_source_name );
        }
    }

    return result;
}

rsai::markup::tile_balancer::tile_balancer ( const tiles &tls )
    : m_tiles ( tls )
{
}

tiles rsai::markup::tile_balancer::operator () ( const double populated_balance ) const
{
    // Index arrays for populated and not populated tiles
    std::vector<int> indices_populated, indices_not_populated;
    indices_populated.reserve ( m_tiles.size() );
    indices_not_populated.reserve ( m_tiles.size() );

    int index = 0;
    int populated_count = 0;
    for ( const auto &tile : m_tiles )
    {
        if ( tile.populated() )
        {
            ++populated_count;
            indices_populated.push_back ( index );
        }
        else
            indices_not_populated.push_back ( index );

        ++index;
    }

    // Balancing part sizes
    int not_populated_count = m_tiles.size () - populated_count;

    if ( not_populated_count * ( 1.0 - populated_balance ) > populated_count * populated_balance )
        not_populated_count = populated_count / populated_balance - populated_count;
    else
        populated_count = not_populated_count / ( 1.0 - populated_balance ) - not_populated_count;

    // Shuffling and resizing index arrays
    std::random_device rd;
    std::mt19937 generator(rd());
    std::shuffle(indices_populated.begin(), indices_populated.end(), generator);
    std::shuffle(indices_not_populated.begin(), indices_not_populated.end(), generator);

    indices_populated.resize ( populated_count );
    indices_not_populated.resize ( not_populated_count );

    std::sort(indices_populated.begin(), indices_populated.end());
    std::sort(indices_not_populated.begin(), indices_not_populated.end());

    // Merging resulting list
    tiles result;
    for (int index : indices_populated)
    {
        auto it = m_tiles.begin ();
        std::advance ( it, index );
        result.push_back ( *it );
    }

    for (int index : indices_not_populated)
    {
        auto it = m_tiles.begin ();
        std::advance ( it, index );
        result.push_back ( *it );
    }

    return std::move ( result );
}

rsai::markup::tile_splitter::tile_splitter ( const tiles &tls, const double validation )
{
    const int training_size = tls.size() * ( 1.0 - validation );
    const int validation_size = tls.size() - training_size;

    std::vector<int> indices ( tls.size() );
    std::iota ( indices.begin(), indices.end(), 0 );

    std::random_device rd;
    std::mt19937 generator(rd());
    std::shuffle(indices.begin(), indices.end(), generator);

    for (int index : indices)
    {
        auto it = tls.begin ();
        std::advance ( it, index );

        if ( m_train.size () < training_size )
            m_train.push_back ( *it );
        else
            m_validation.push_back ( *it );
    }
}

tiles rsai::markup::tile_splitter::train () const
{
    return std::move ( m_train );
}

tiles rsai::markup::tile_splitter::validation () const
{
    return std::move ( m_validation );
}
