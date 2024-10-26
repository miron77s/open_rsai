#pragma once

#include <string>
#include <exception>
#include <Eigen/Dense>

namespace eigen
{
    template < typename Type, int Size >
    class vector_helper
    {
    public:
        using vector_type = Eigen::Matrix < Type, Size, 1 >;
        vector_helper ( const std::string &src, const std::string &delimiters );

        template < class Out >
        bool                    verify ( Out &out );
        vector_type             value () const;

    private:
        std::string _src;
        std::string _delimiters;
        vector_type _vector = vector_type::Zero ();
    }; // class vector_helper

    template < typename Type, int Size >
    Eigen::Matrix < Type, Size + 1, 1 > extend ( const Eigen::Matrix < Type, Size, 1 > &what );

    template < typename Type, int Size >
    Eigen::Matrix < Type, Size - 1, 1 > shrink ( const Eigen::Matrix < Type, Size, 1 > &what );
}; // namespace eigen

#include "eigen_utils/vector_helpers.hpp"
