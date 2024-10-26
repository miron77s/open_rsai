#pragma once

#include "convolution_mask.h"
#include <map>
#include <cstdint>

template < class DataType >
convolution_mask_t < DataType >::convolution_mask_t ()
    : m_half_width ( 0 )
    , m_half_height ( 0 )
    , m_scale ( 0 )
    , m_shift ( 0.0 )
{
}

template < class DataType >
template < class Function >
convolution_mask_t < DataType >::convolution_mask_t ( const Function &function, const int halfWidth, const int halfHeight, const double zeroThreshold )
    : m_half_width ( halfWidth )
    , m_half_height ( halfHeight )
    , m_shift ( 0.0 )
{
    struct ItemLocal
    {
        double value;
        Point2i at;
    };

    using ItemsLocal = std::list < ItemLocal >;

    // Расчет откликов функции, определение минимума среди превышающих пороговое значение
    double minimal_respose = 1e+300;
    ItemsLocal items;
    for ( int y = -halfHeight; y <= halfHeight; ++y )
    {
        for ( int x = -halfWidth; x <= halfWidth; ++x )
        {
            const double response = function ( x, y ),
                         responseAbs = std::abs ( response );
            if ( responseAbs > zeroThreshold )
            {
                minimal_respose = std::min ( minimal_respose, responseAbs );
                items.push_back ( { response, Point2i ( x, y ) } );
            }

        }
    }

    // Сохранение откликов с масшабированием до целых чисел
    for ( auto & itemLocal : items )
    {
        m_items.push_back ( { static_cast < DataType > ( std::round ( itemLocal.value / minimal_respose ) ), itemLocal.at } );
    }

    // расчет масштабного коэффициента
    int64_t positive_sum = 0, negative_sum = 0;
    for ( auto & item : m_items )
    {
        if ( item.value > 0 )
            positive_sum += item.value;
        else
            negative_sum -= item.value;
    }

    m_scale = std::max ( positive_sum, negative_sum );
}

template < class DataType >
template < class Function >
convolution_mask_t < DataType >::convolution_mask_t ( const Function &function, const int half_size, const double zero_threshold )
    : m_half_width ( half_size )
    , m_half_height ( half_size )
{
    struct item_local
    {
        double value;
        Point2i at;
    };

    using items_local = std::list < item_local >;

    // Расчет откликов функции, определение минимума среди превышающих пороговое значение
    const int radius2 = half_size * half_size;

    double minimal_respose = 1e+300;
    items_local items;
    for ( int y = -half_size; y <= half_size; ++y )
    {
        for ( int x = -half_size; x <= half_size; ++x )
        {
            const int dist2 = x * x + y * y;
            if ( dist2 > radius2 )
                continue;

            const double response = function ( x, y ),
                         responseAbs = std::abs ( response );
            if ( responseAbs > zero_threshold )
            {
                minimal_respose = std::min ( minimal_respose, responseAbs );
                items.push_back ( { response, Point2i ( x, y ) } );
            }

        }
    }

    // Сохранение откликов с масшабированием до целых чисел
    for ( auto & itemLocal : items )
    {
        m_items.push_back ( { qRound ( itemLocal.value / minimal_respose ), itemLocal.at } );
    }

    // расчет масштабного коэффициента
    long long positiveSum = 0, negativeSum = 0;
    for ( auto & item : m_items )
    {
        if ( item.value > 0 )
            positiveSum += item.value;
        else
            negativeSum -= item.value;
    }

    m_scale = std::max ( positiveSum, negativeSum );
    m_shift = 127.0 * ( negativeSum - positiveSum ) / std::min ( positiveSum, negativeSum );
}

template < class DataType >
auto convolution_mask_t < DataType >::begin () const
{
    return m_items.begin ();
}

template < class DataType >
auto convolution_mask_t < DataType >::end () const
{
    return m_items.end ();
}

template < class DataType >
const DataType convolution_mask_t < DataType >::scale_factor () const
{
    return m_scale;
}

template < class DataType >
const double convolution_mask_t < DataType >::shift () const
{
    return m_shift;
}

template < class DataType >
template < class MatType >
inline const DataType convolution_mask_t < DataType >::operator () ( const Mat &what, const Point2i &at ) const
{
    return this->operator() < DataType > ( what, at.x, at.y );
}

template < class DataType >
template < class MatType >
inline const DataType convolution_mask_t < DataType >::operator () ( const Mat &what, const int x, const int y ) const
{
    if ( scale_factor () == 0 )
        return 0;

    DataType response = 0;
    for ( auto & item : m_items )
    {
        const auto px = x + item.at.x,
                   py = y + item.at.y;
        response += what.at < MatType > ( py, px ) * item.value;
    }

    return std::round ( response / static_cast < double > ( scale_factor () ) + shift () );
}

template < class DataType >
template < class MatType, class OutMatType, class MaskType >
Mat convolution_mask_t < DataType >::conv ( const Mat &what, const int outMatType, const Mat &mask ) const
{
    Mat out = Mat::zeros ( what.size (), outMatType );

    const int yFrom = m_half_height, yTo = what.rows - m_half_height,
              xFrom = m_half_width, xTo = what.cols - m_half_width;
    for ( int y = yFrom; y < yTo; ++y )
    {
        if ( !mask.empty() )
        {
            for ( int x = xFrom; x < xTo; ++x )
            {
                if ( mask.at < MaskType > ( y, x ) )
                    out.at < OutMatType > ( y, x ) = operator () < MatType > ( what, x, y );
            }
        }
        else
            for ( int x = xFrom; x < xTo; ++x )
                out.at < OutMatType > ( y, x ) = operator () < MatType > ( what, x, y );
    }

    return out;
}

template < class DataType >
template < class MatType, class OutMatType >
Mat convolution_mask_t < DataType >::sparsed_conv ( const Mat &what, const int outMatType, const Point2i &step ) const
{
    Mat out = Mat::zeros ( what.size (), outMatType );

    const int y_from = m_half_height, yTo = what.rows - m_half_height,
              x_from = m_half_width, xTo = what.cols - m_half_width;
    for ( int y = y_from; y < yTo; y += step.y )
    {
        for ( int x = x_from; x < xTo; x += step.x )
        {
            auto value = operator () < MatType > ( what, x, y );

            for ( int i = 0; i < step.y; ++i )
            {
                for ( int j = 0; j < step.x; ++j )
                    out.at < OutMatType > ( y + i, x + j ) = value;
            }
        }
    }

    return out;
}

template < class DataType >
Size convolution_mask_t < DataType >::size () const
{
    return { m_half_width * 2 + 1, m_half_height * 2 + 1 };
}

template < class DataType >
Size convolution_mask_t < DataType >::semi_size () const
{
    return { m_half_width, m_half_height };
}

template < class StreamType, class DataType >
StreamType & operator << ( StreamType &stream, const convolution_mask_t < DataType > &mask )
{
    std::map < int, std::map < int, double > > regilar_mask;
    for ( auto item : mask )
    {
        regilar_mask [item.at.y][item.at.x] = item.value;
    }

    auto semi_size = mask.semi_size();

    for ( int y = -semi_size.height; y <= semi_size.height; ++y )
    {
        for ( int x = -semi_size.width; x <= semi_size.width; ++x )
            stream << regilar_mask [y][x] << "\t";
        stream << std::endl;
    }

    return stream;
}
