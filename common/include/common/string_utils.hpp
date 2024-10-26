#include "common/string_utils.h"

static inline std::vector < std::string > split ( const std::string &s, char delim )
{
    std::vector < std::string > result;
    std::stringstream ss (s);
    std::string item;

    while ( getline (ss, item, delim ) )
    {
        result.push_back ( item );
    }

    return std::move ( result );
}

// trim from start (in place)
static inline void ltrim (std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
    {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s)
{
    rtrim(s);
    ltrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s)
{
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s)
{
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s)
{
    trim(s);
    return s;
}

template < typename Type >
inline Type value_from_string ( const std::string &s )
{
    return std::stof ( s );
}

template <>
inline int value_from_string < int > ( const std::string &s )
{
    return std::stoi ( s );
}

template <>
inline long value_from_string < long > ( const std::string &s )
{
    return std::stol ( s );
}

template <>
inline unsigned long value_from_string < unsigned long > ( const std::string &s )
{
    return std::stoul ( s );
}

template <>
inline long long value_from_string < long long > ( const std::string &s )
{
    return std::stoll ( s );
}

template <>
inline unsigned long long value_from_string < unsigned long long > ( const std::string &s )
{
    return std::stoull ( s );
}

template <>
inline double value_from_string < double > ( const std::string &s )
{
    return std::stod ( s );
}

template <>
inline std::string value_from_string < std::string > ( const std::string &s )
{
    return s;
}

template < class Type >
value_helper < Type >::value_helper ( const std::string &src )
    : _src ( src )
{
}

template < class Type >
template < class Out >
bool value_helper < Type >::verify ( Out &out, const std::string &error_msg )
{
    try
    {
        _value = value_from_string < Type > ( _src );
    }
    catch ( ... )
    {
        out << "Value '" << _src << "' is invalid" << std::endl << error_msg << std::endl;
        return false;
    }
    return true;
}

template < class Type >
Type value_helper < Type >::value () const
{
    return _value;
}

template < class Type >
array_helper < Type >::array_helper ( const std::string &src, const std::string delims )
    : _src ( src ), _delimiters ( delims )
{
}

template < class Type >
template < class Out >
bool array_helper < Type >::verify ( Out &out, const std::string &error_msg, char delim )
{
    std::vector < std::string > items;

    for ( auto del_variant : _delimiters )
    {
        items = split ( _src, del_variant );
        if ( items.size () > 1 )
            break;
    }

    for ( auto item_str : items )
    {
        trim ( item_str );
        value_helper < Type > helper ( item_str );
        if ( helper.verify ( out, error_msg ) )
            _array.emplace_back ( helper.value() );
        else
            return false;
    }
    return true;
}

template < class Type >
typename array_helper < Type >::array_type array_helper < Type >::value () const
{
    return _array;
}
