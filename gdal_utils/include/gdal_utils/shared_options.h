#pragma once

#include <string>
#include <utility>

namespace gdal
{
    using key_value = std::pair < std::string, std::string >;

    class shared_options
    {
    public:
        shared_options ();
        shared_options ( char ** options );
        ~shared_options ();

        void        set                 ( const std::string &key, const std::string &value );
        char **     get                 () const;

        const shared_options & operator << ( const key_value & kvp );

        void        set_encoding        ( const std::string &value );
        void        set_destination_srs ( const std::string &value );

    private:
        char ** _options = nullptr;

        shared_options ( const shared_options &src )    = delete;
        shared_options ( shared_options &&src )         = delete;
    };
}; //namespace gdal
