#pragma once

#include <sstream>
#include <algorithm>
#include <cctype>
#include <locale>
#include <vector>
#include "common/definitions.h"

using strings = std::vector < std::string >;

static inline std::vector < std::string > split ( const std::string &s, char delim );

// trim from start (in place)
static inline void ltrim (std::string &s);

// trim from end (in place)
static inline void rtrim(std::string &s);

// trim from both ends (in place)
static inline void trim(std::string &s);

// trim from start (copying)
static inline std::string ltrim_copy(std::string s);

// trim from end (copying)
static inline std::string rtrim_copy(std::string s);

// trim from both ends (copying)
static inline std::string trim_copy(std::string s);

template < typename Type >
inline Type value_from_string ( const std::string &s );

template <>
inline int value_from_string < int > ( const std::string &s );

template <>
inline long value_from_string < long > ( const std::string &s );

template <>
inline unsigned long value_from_string < unsigned long > ( const std::string &s );

template <>
inline long long value_from_string < long long > ( const std::string &s );
template <>
inline unsigned long long value_from_string < unsigned long long > ( const std::string &s );

template <>
inline double value_from_string < double > ( const std::string &s );

template <>
inline std::string value_from_string < std::string > ( const std::string &s );

template < class Type >
class value_helper
{
public:
    value_helper ( const std::string &src );

    template < class Out >
    bool        verify ( Out &out, const std::string &error_msg );
    Type        value () const;

private:
    std::string _src;
    Type    _value;
}; // class value_helper

template < class Type >
class array_helper
{
public:
    using array_type = std::vector < Type >;

    array_helper ( const std::string &src, const std::string delims = DEFAULT_DELIMITERS );

    template < class Out >
    bool        verify ( Out &out, const std::string &error_msg, char delim = ',' );
    array_type  value () const;

private:
    std::string _src;
    std::string _delimiters;
    array_type  _array;
}; // class value_helper

#include "common/string_utils.hpp"
