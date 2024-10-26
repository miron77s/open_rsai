#include "differentiation/gauss_directed_derivative.h"

using namespace gauss;

function::function ( const double sigma, const bool normed, const double span )
    : m_2sigma2 ( 2.0 * sigma * sigma )
    , m_factor ( 1.0 / ( ( normed ) ? sqrt ( 2.0 * CV_PI ) * sigma : 1.0 ) )
    , m_span2 ( span * span )
{

}

first_directed_derivative::first_directed_derivative ( const double angle_rad, const double sigma, const double span )
    : m_sin ( sin ( angle_rad ) )
    , m_cos ( cos ( angle_rad ) )
    , m_2sigma2 ( 2.0 * sigma * sigma )
    , m_factor ( 1.0 / ( sqrt ( 2.0 * CV_PI ) * sigma * sigma * sigma ) )
    , m_span ( span )
{
}

second_directed_derivative::second_directed_derivative ( const double angle_rad, const double sigma, const double span )
    : m_sin ( sin ( angle_rad ) )
    , m_cos ( cos ( angle_rad ) )
    , m_sigma2 ( sigma * sigma )
    , m_2sigma2 ( 2.0 * sigma * sigma )
    , m_factor ( 1.0 / ( sqrt ( 2.0 * CV_PI ) * sigma * sigma * sigma ) )
    , m_span ( span )
{
}
