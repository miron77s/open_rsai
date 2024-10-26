#pragma once

#include "common/string_utils.h"
#include "rsai/markup/tile.h"

#include <memory>
#include "common/progress_functions.hpp"

namespace rsai
{

    enum class write_mode
    {
        invalid,
        replace,
        append,
        update
    };

    namespace markup
    {
        enum class markup_type
        {
            invalid,
            yolo,
            coco
        };

        enum class markup_part
        {
            train,
            valid
        };

        class writer;
        using writer_ptr = std::shared_ptr < writer >;

        class writer
        {
        public:
            writer ( const strings &classes );
            virtual bool save ( const tiles &tls, const std::string &path, const markup_part part, write_mode mode = write_mode::append, bool show_progress = true ) const = 0;

            static writer_ptr get_writer ( const markup_type type, const strings &classes );

        protected:
            std::map < std::string, int > m_classes;
        };

        class yolo_writer
                : public writer
        {
        public:
            yolo_writer ( const strings &classes );
            virtual bool save ( const tiles &tls, const std::string &path, const markup_part part, write_mode mode = write_mode::append, bool show_progress = true ) const override;

        private:
            std::string __geometry_to_yolo(OGRGeometry *geometry, int tile_width, int tile_height, int class_id) const;
            std::string __geometries_to_yolo(const gdal::geometries& geometries, int tile_width, int tile_height, int class_id) const;
            bool        __write_class_names ( const std::string &path ) const;
            bool        __write_data_file   ( const std::string &path ) const;
        };
    } // namespace markup

} // namespace rsai
