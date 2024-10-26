#include "eigen_utils/vector_helpers.h"

#include <vector>

#include "common/string_utils.h"

template < typename Type, int Size >
eigen::vector_helper < Type, Size >::vector_helper ( const std::string &src, const std::string &delimiters )
    : _src ( src ), _delimiters ( delimiters )
{
}

template < typename Type, int Size >
template < class Out >
bool eigen::vector_helper < Type, Size >::verify ( Out &out )
{
    std::vector < std::string > items;

    for ( auto del_variant : _delimiters )
    {
        items = split ( _src, del_variant );
        if ( items.size () > 1 )
            break;
    }

    if ( items.size () <= 1 )
    {
        out << "Failed to split vector '" << _src << "' with the set of delimiters '" << _delimiters << "'" << std::endl;
        return false;
    }

    if ( items.size () != Size )
    {
        out << "Items count in input vector '" << _src << " missmatches the desired " << Size << "-dimensional vector" << std::endl;
        return false;
    }

    // In case of invalid_argument or out_of_range exceptions
    try
    {
        for ( int i = 0; i < items.size (); ++i )
        {
            auto &item = items [i];
            trim ( item );
            _vector [i] = value_from_string < Type > ( item );
        }
    }
    catch ( ... )
    {
        out << "Vector '" << _src << "' contains invalid values" << std::endl;
        _vector = vector_type::Zero ();
        return false;
    }


    return true;
}

template < typename Type, int Size >
Eigen::Matrix < Type, Size, 1 > eigen::vector_helper < Type, Size >::value () const
{
    return _vector;
}

template < typename Type, int Size >
Eigen::Matrix < Type, Size + 1, 1 > eigen::extend ( const Eigen::Matrix < Type, Size, 1 > &what )
{
    Eigen::Matrix < Type, Size + 1, 1 > result;
    for ( int i = 0; i < Size; ++i )
        result [i] = what [i];
    result [Size] = 1;
    return std::move ( result );
}

template < typename Type, int Size >
Eigen::Matrix < Type, Size - 1, 1 > eigen::shrink ( const Eigen::Matrix < Type, Size, 1 > &what )
{
    Eigen::Matrix < Type, Size - 1, 1 > result;
    for ( int i = 0; i < Size - 1; ++i )
        result [i] = what [i];
    return std::move ( result );
}
