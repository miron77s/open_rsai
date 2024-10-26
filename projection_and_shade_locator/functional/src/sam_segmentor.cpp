#include "rsai/sam_segmentor.h"
#include "opencv_utils/raster_roi.h"

#include <filesystem>

using namespace rsai;

sam_tile::sam_tile ( gdal::shared_dataset raster, const std::string &path, const int index, const Eigen::Vector2d &pos, const Eigen::Vector2d &size )
    : m_index ( index ), m_pos ( pos ), m_size ( size ), m_center ( pos + size / 2 )
    , m_file_name ( path + std::to_string ( m_index ) + DEFAULT_SEGMENT_FILE_EXT )
    , m_segs_name ( path + std::to_string ( m_index ) + DEFAULT_SEGMENT_WKT_FILE_EXT )
{
    opencv::dataset_roi_extractor ds_tile_extractor ( raster );

    //std::cout << "Created tile index " << m_index << " filename " << m_file_name << " segs name " << m_segs_name << '\n';

    gdal::bbox raster_box ( pos, pos + size );
    auto tile = ds_tile_extractor.roi ( raster_box );

    m_pos = raster_box.top_left();
    m_size = raster_box.size ();
    m_center = raster_box.center();

    cv::imwrite ( m_file_name, tile );
}

gdal::bbox sam_tile::bbox () const
{
    return { m_pos, m_pos + m_size };
}

bool sam_tile::verify ( const gdal::bbox &tile_bbox ) const
{
    const Eigen::Vector2d tile_tl = tile_bbox.top_left(),
                          tile_br = tile_bbox.bottom_right();

    const Eigen::Vector2d seg_tl = m_pos,
                          seg_br = m_pos + m_size;

    return tile_tl.x() >= seg_tl.x() && tile_br.x() <= seg_br.x() && tile_tl.y() >= seg_tl.y() && tile_br.y() <= seg_br.y();
}

gdal::polygons sam_tile::segments ( const gdal::bbox &tile_bbox )
{
    auto local_bbox = tile_bbox;
    local_bbox += -m_pos;

    //std::cout << "Getting segments tile " << tile_bbox << " from " << m_pos << " and local bbox " << local_bbox << '\n';
    //std::cout.flush ();

    auto segs = to_polygon ( gdal::from_wkt_file ( m_segs_name, local_bbox.to_polygon() ) );
    segs += -local_bbox.top_left();

    return segs;
}

/*cv::Mat sam_tile::segments ( const gdal::bbox &tile_bbox )
{
    if ( m_segs.empty() )
        m_segs = cv::imread ( m_segs_name );

    const Eigen::Vector2d tile_tl = tile_bbox.top_left(),
                          tile_br = tile_bbox.bottom_right(),
                          tile_size = tile_br - tile_tl + Eigen::Vector2d ( 1.0, 1.0 );

    const auto relative_tl = cv::Point ( std::round < int > ( tile_tl.x() ), std::round < int > ( tile_tl.y() ) ) - m_pos;
    const auto tile_size_cv = cv::Size ( std::round < int > ( tile_size.x() ), std::round < int > ( tile_size.y () ) );

    if ( !m_segs.empty() )
        return m_segs ( cv::Rect ( relative_tl, tile_size_cv ) ).clone();
    else
        return {};
}*/

double sam_tile::distance ( const gdal::bbox &tile_bbox ) const
{
    auto bbox_center = tile_bbox.center();
    return ( m_center - bbox_center ).norm();
}

sam_tiles::sam_tiles ( gdal::shared_dataset raster, const std::string &path )
    : m_raster ( raster ), m_path ( path ), m_index ( 0 )
{
}

sam_tiles::sam_tiles ( gdal::shared_dataset raster, const std::string &path, gdal::geometry coverage, const Eigen::Vector2d tile_size, const Eigen::Vector2d tile_step )
    : m_raster ( raster ), m_path ( path ), m_index ( 0 )
{
    OGREnvelope envelope;
    coverage->getEnvelope(&envelope);

    const int from_x = std::floor < int > ( envelope.MinX );
    const int from_y = std::floor < int > ( envelope.MinY );
    const int to_x = std::floor < int > ( envelope.MaxX );
    const int to_y = std::floor < int > ( envelope.MaxY );

    for (int y = from_y; y < to_y; y += tile_step.y())
    {
        for (int x = from_x; x < to_x; x += tile_step.x())
        {
            OGRPolygon rectangle;
            auto * ring = new OGRLinearRing;
            ring->addPoint(x, y);
            ring->addPoint(x + tile_size.x(), y);
            ring->addPoint(x + tile_size.x(), y + tile_size.y());
            ring->addPoint(x, y + tile_size.y());
            rectangle.addRingDirectly(ring);
            rectangle.closeRings();

            if (rectangle.Intersects(coverage.get()))
            {
                std::scoped_lock lock_guard ( m_lock );
                m_tiles.emplace_back ( raster, path, m_index++, Eigen::Vector2d ( x, y ), tile_size );
            }
        }
    }
}

gdal::polygons sam_tiles::segments ( const gdal::bbox &tile_bbox )
{
    std::pair < int, double > best_distance ( -1, 1e+30 );
    for ( int i = 0; i < m_tiles.size(); ++i )
    {
        const auto & tile = m_tiles [i];

        if ( !tile.verify( tile_bbox ) )
            continue;

        const auto curr_distance = tile.distance ( tile_bbox );
        if ( best_distance.second > curr_distance )
            best_distance = { i, curr_distance };
    }

    if ( best_distance.first >= 0 )
    {
        auto & best_tile = m_tiles [best_distance.first];
        //std::cout << "best seg for tile " << tile_bbox << " is no " << best_distance.first << " " << best_tile.bbox() << " with dist = " << best_distance.second << '\n';
        return best_tile.segments ( tile_bbox );
    }
    else
        return {};
}

/*cv::Mat sam_tiles::segments ( const gdal::bbox &tile_bbox )
{
    std::pair < int, double > best_distance ( -1, 1e+30 );
    for ( int i = 0; i < m_tiles.size(); ++i )
    {
        const auto & tile = m_tiles [i];
        const auto curr_distance = tile.distance ( tile_bbox );
        if ( best_distance.second > curr_distance )
            best_distance = { i, curr_distance };
    }

    if ( best_distance.first >= 0 )
    {
        auto & best_tile = m_tiles [best_distance.first];
        return best_tile.segments ( tile_bbox );
    }
    else
        return {};
}*/

bool sam_tiles::verify_or_add ( const gdal::bbox &tile_bbox )
{
    std::scoped_lock lock_guard ( m_lock );

    std::pair < int, double > best_distance ( -1, 1e+30 );
    for ( int i = 0; i < m_tiles.size(); ++i )
    {
        const auto & tile = m_tiles [i];

        if ( !tile.verify( tile_bbox ) )
            continue;

        const auto curr_distance = tile.distance ( tile_bbox );
        if ( best_distance.second > curr_distance )
            best_distance = { i, curr_distance };
    }

    bool found_tile = best_distance.first >= 0;

    if ( !found_tile )
    {
        const Eigen::Vector2d tile_tl = tile_bbox.top_left() - Eigen::Vector2d { 100, 100 },
                              tile_br = tile_bbox.bottom_right() + Eigen::Vector2d { 100, 100 },
                              tile_size = tile_br - tile_tl;

        m_tiles.emplace_back ( m_raster, m_path, m_index++, tile_tl, tile_size );
    }

    return !found_tile;
}

rsai::sam_segmentor::sam_segmentor ( gdal::shared_dataset raster, gdal::shared_dataset objects_vector, const std::string &store_dir )
    : m_raster ( raster ), m_objects_vector ( objects_vector ), m_store_dir ( store_dir )
{
}

rsai::sam_segmentor::~sam_segmentor ()
{
    //std::filesystem::remove_all ( m_store_dir );
    //std::filesystem::remove ( m_store_dir );
}
