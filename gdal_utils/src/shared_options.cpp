#include "gdal_utils/shared_options.h"

#include <gdal_priv.h>

const std::string encoding_key          = "ENCODING",
                  destination_srs_key   = "DST_SRSWKT";

gdal::shared_options::shared_options ()
    : _options ( nullptr )
{

}

gdal::shared_options::shared_options ( char ** options )
    : _options ( options )
{

}

gdal::shared_options::~shared_options ()
{
    if ( _options != nullptr )
        CSLDestroy ( _options );
}

void gdal::shared_options::set ( const std::string &key, const std::string &value )
{
    _options = CSLSetNameValue ( _options, key.c_str (), value.c_str () );
}

char ** gdal::shared_options::get () const
{
    return _options;
}

const gdal::shared_options & gdal::shared_options::operator << ( const key_value & kvp )
{
    set ( kvp.first, kvp.second );
    return *this;
}

void gdal::shared_options::set_encoding ( const std::string &value )
{
    set ( encoding_key, value );
}

void gdal::shared_options::set_destination_srs ( const std::string &value )
{
    set ( destination_srs_key, value );
}
