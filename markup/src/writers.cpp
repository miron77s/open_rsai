#include "rsai/markup/writers.h"

#include <filesystem>

using namespace rsai::markup;

std::map < markup_part, std::string > markup_part_folder_names = { { markup_part::train, "train" }, { markup_part::valid, "valid" } };

rsai::markup::writer::writer ( const strings &classes )
{
    for ( int i = 0; i < classes.size(); ++i )
        m_classes [classes [i]] = i;
}

writer_ptr rsai::markup::writer::get_writer ( const markup_type type, const strings &classes )
{
    if ( type == markup_type::yolo )
        return writer_ptr ( new yolo_writer ( classes ) );

    return {};
}

rsai::markup::yolo_writer::yolo_writer ( const strings &classes )
    : writer ( classes )
{
}

bool rsai::markup::yolo_writer::save ( const tiles &tls, const std::string &path, const markup_part part, write_mode mode, bool show_progress ) const
{
    const std::string images_ext = ".jpg";
    const std::string markup_ext = ".txt";

    const auto dataset_name = std::filesystem::path ( path ).filename().string();

    const auto markup_dir = markup_part_folder_names [part];
    const std::string tiles_folder = markup_dir;
    const auto tiles_path = path + "/" + tiles_folder + "/";

    if ( mode == write_mode::replace )
        std::filesystem::remove_all ( path );

    if ( !std::filesystem::exists ( tiles_path ) )
        std::filesystem::create_directories( tiles_path );

    const auto index_path = path + "/" + markup_dir + markup_ext;
    std::ofstream index_file ( index_path, ( mode == write_mode::append ) ? std::ios_base::app : std::ios_base::trunc );
    if (!index_file.is_open())
    {
        std::cerr << "Failed to open the file for writing: " << index_path << std::endl;
        return false;
    }

    int tile_index = 0;
    for ( auto & tile : tls )
    {
        const auto image_file_name = tile.source() + "_" + std::to_string ( tile.index() ) + images_ext;
        const auto image_path = tiles_path + image_file_name;
        const auto markup_path = tiles_path + tile.source() + "_" + std::to_string ( tile.index() ) + markup_ext;
        auto image = tile.image();
        cv::imwrite ( image_path, image );

        auto & tile_population = tile.population();

        std::ofstream file(markup_path);
        if (!file.is_open())
        {
            std::cerr << "Failed to open the file for writing: " << markup_path << std::endl;
            return false;
        }

        for ( auto &class_population : tile_population )
        {
            auto class_name = class_population.first;
            auto class_index = ( m_classes.find( class_name ) )->second;

            auto yolo_markup = __geometries_to_yolo ( class_population.second, image.cols, image.rows, class_index );
            file << yolo_markup;
        }

        const auto image_relative_path = dataset_name + "/" + markup_dir + "/" + image_file_name;
        index_file << image_relative_path << '\n';

        tile_index++;

        file.close();

        if ( show_progress )
            console_progress ( float ( tile_index ) / tls.size() );
    }

    index_file.close ();

    if ( show_progress )
        console_progress ( 1.f, true );

    if ( !__write_class_names ( path ) )
        return false;

    return __write_data_file ( path );
}

std::string rsai::markup::yolo_writer::__geometry_to_yolo ( OGRGeometry *geometry, int tile_width, int tile_height, int class_id ) const
{
    OGREnvelope envelope;
    geometry->getEnvelope(&envelope);

    // Trimming envelope by tile window
    envelope.MinX = ( envelope.MinX < 0 ) ? 0 : envelope.MinX;
    envelope.MinY = ( envelope.MinY < 0 ) ? 0 : envelope.MinY;
    envelope.MaxX = ( envelope.MaxX >= tile_width ) ? tile_width - 1 : envelope.MaxX;
    envelope.MaxY = ( envelope.MaxY >= tile_height ) ? tile_height - 1 : envelope.MaxY;

    if ( envelope.MinX > envelope.MaxX || envelope.MinY > envelope.MaxY )
        return {};

    // Calculate the center, width, and height of the bounding box
    double x_center = (envelope.MinX + envelope.MaxX) / 2.0;
    double y_center = (envelope.MinY + envelope.MaxY) / 2.0;
    double width = envelope.MaxX - envelope.MinX;
    double height = envelope.MaxY - envelope.MinY;

    // Normalize the values to be relative to the tile size
    x_center /= tile_width;
    y_center /= tile_height;
    width /= tile_width;
    height /= tile_height;

    // Format the string in YOLO format using stringstream
    std::ostringstream stream;
    stream << class_id << " "
           << std::fixed << std::setprecision(6)
           << x_center << " "
           << y_center << " "
           << width << " "
           << height;

    return stream.str();
}

std::string rsai::markup::yolo_writer::__geometries_to_yolo(const gdal::geometries& geometries, int tile_width, int tile_height, int class_id) const
{
    std::ostringstream stream;
    for (auto geometry : geometries)
    {
        std::string yolo_line = __geometry_to_yolo(geometry.get (), tile_width, tile_height, class_id);

        if ( !yolo_line.empty() )
            stream << yolo_line << std::endl;
    }

    return stream.str();
}

bool rsai::markup::yolo_writer::__write_class_names ( const std::string &path ) const
{
    const std::string class_names_file = "rsai.names";
    const std::string class_names_path = path + "/" + class_names_file;
    std::ofstream file (class_names_path);
    if ( !file.is_open () )
    {
        std::cerr << "Failed to open the file for writing: " << class_names_path << std::endl;
        return false;
    }

    strings classes ( m_classes.size () );
    for ( const auto & a_class : m_classes )
        classes [a_class.second] = a_class.first;

    for ( const auto & a_class : classes )
        file << a_class << '\n';

    file.close ();

    return true;
}

bool rsai::markup::yolo_writer::__write_data_file ( const std::string &path ) const
{
    const std::string index_ext = ".txt";
    const std::string data_file = "rsai.data";
    const std::string data_path = path + "/" + data_file;
    const auto dataset_name = std::filesystem::path ( path ).filename().string();
    std::ofstream file (data_path);
    if ( !file.is_open () )
    {
        std::cerr << "Failed to open the file for writing: " << data_file << std::endl;
        return false;
    }

    file << "classes = " << m_classes.size() << '\n'
         << "train = "  << dataset_name << "/" << markup_part_folder_names [markup_part::train] << index_ext << '\n'
         << "valid = "  << dataset_name << "/" << markup_part_folder_names [markup_part::valid] << index_ext << '\n'
         << "names = "  << dataset_name << "/" << "rsai.names" << '\n'
         << "backup = " << dataset_name << "/" << "backup/";
    file.close ();

    return true;
}
