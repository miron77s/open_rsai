#pragma once

#include <Eigen/Dense>

namespace eigen
{

    template < class Type >
    Eigen::Vector2 < Type > to_polar (const Eigen::Vector2 < Type >& vec)
    {
        const double r = vec.norm();
        double theta = std::atan2(vec(1), vec(0));
        theta = ( theta < 0.0 ) ? M_PI + theta : theta;
        return { r, theta };
    }

    template < class Type >
    Eigen::Vector2 < Type > to_polar2d (const Eigen::Vector3 < Type >& vec)
    {
        const double r = vec.norm();
        double theta = std::atan2(vec(1), vec(0));
        theta = ( theta < 0.0 ) ? M_PI + theta : theta;
        return { r, theta };
    }

}; // namespace eigen
