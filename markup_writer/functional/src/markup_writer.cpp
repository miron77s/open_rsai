#include "rsai/markup_writer.hpp"

using namespace rsai::markup;

write_mode rsai::write_mode_from_string ( const std::string &mode )
{
    static std::map < std::string, write_mode > mode_mapping = { { "replace", write_mode::replace }, { "append", write_mode::append }, { "update", write_mode::update } };
    return mode_mapping [mode];
}

rsai::markup::markup_type rsai::markup_type_from_string ( const std::string &type )
{
    static std::map < std::string, markup_type > type_mapping = { { "yolo", markup_type::yolo }, { "coco", markup_type::coco } };
    return type_mapping [type];
}
