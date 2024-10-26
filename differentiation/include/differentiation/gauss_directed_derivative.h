#pragma once

/*!
    \file
    \brief Направленная производная функции Гаусса - двумерный вариант одномерной функции, ориентированный в заданом направлении
    \author Мирошниченко С.Ю.
    \version 1.0
*/

#include <opencv2/opencv.hpp>

using namespace cv;

/**
* \brief	Пространство имен объединяет различные производные функции
*/
namespace gauss
{

    /**
    * \brief	Функция Гаусса
    */
    class function
    {
    public:

        /**
        * \brief	Создать на основе размаха sigma
        * \param 	normed При значении true сумма отлика равна единице, при false - центральный элемент имеет значение 1.0
        */
        function ( const double sigma = 1.0, const bool normed = true, const double span = 10e+7 );

        /**
        * \brief	Расчитатить значение функции в заданной точке
        * \tparam 	CoordType   Тип координат для расчета значения. Позволяет использовать как дискретные, так и непрерывные сетки
        * \param 	at          Положение интересующей точки
        * \return 	Значение производной в заданной параметром точке
        */
        template < class CoordType >
        double operator () ( const Point_ < CoordType > &at ) const;

        /**
        * \brief	Расчитатить значение функции в заданной двумя координатами точке
        * \tparam 	CoordType   Тип координат для расчета значения. Позволяет использовать как дискретные, так и непрерывные сетки
        * \param 	x           Горизонтальное смещение точки
        * \param 	у           Вертикальное смещение точки
        * \return 	Значение производной в заданной параметром точке
        */
        template < class CoordType >
        double operator () ( const CoordType x, const CoordType y ) const;

    private:
        double m_2sigma2 = 2.0;  ///< Предрасчитанное значение 2.0 * sigma * sigma, используется для ускорения вычислений
        double m_factor = 1.0;   ///< Предрасчитанное значение 1.0 / ( sqrt ( 2.0 * CV_PI ) * sigma ), используется для ускорения вычислений
        double m_span2 = 10e+7;  ///< "Размах" отклика функции для реализации КИХ-принципа
    }; // GaussDirecterDerivativeTemplate

    template < class CoordType >
    double function::operator () ( const Point_ < CoordType > &at ) const
    {
        return operator () ( at.x, at.y );
    }

    template < class CoordType >
    double function::operator () ( const CoordType x, const CoordType y ) const
    {
        const double r2 = x * x + y * y;
        return r2 < m_span2 ? m_factor * exp ( - r2 / m_2sigma2 ) : 0.0;
    }

    /**
    * \brief	Первая направленная производная, используется как функтор для создания маски 
    */
    class first_directed_derivative
    {
    public:

        /**
        * \brief	Создать на основе угла поворота в плоскости angle_rad (радианы) и размаха sigma
        */
        first_directed_derivative ( const double angle_rad = 0.0, const double sigma = 1.0, const double span = 10e+7 );

        /**
        * \brief	Расчитатить значение функции в заданной точке
        * \tparam 	CoordType   Тип координат для расчета значения. Позволяет использовать как дискретные, так и непрерывные сетки
        * \param 	at          Положение интересующей точки
        * \return 	Значение производной в заданной параметром точке
        */
        template < class CoordType >
        double operator () ( const Point_ < CoordType > &at ) const;

        /**
        * \brief	Расчитатить значение функции в заданной двумя координатами точке
        * \tparam 	CoordType   Тип координат для расчета значения. Позволяет использовать как дискретные, так и непрерывные сетки
        * \param 	x           Горизонтальное смещение точки
        * \param 	у           Вертикальное смещение точки
        * \return 	Значение производной в заданной параметром точке
        */
        template < class CoordType >
        double operator () ( const CoordType x, const CoordType y ) const;

    private:
        double m_sin = 0.0,  ///< Синус заданного при создании функции угла, используется для ускорения вычислений
               m_cos = 0.0;  ///< Косинус заданного при создании функции угла, используется для ускорения вычислений

        double m_2sigma2 = 2.0;  ///< Предрасчитанное значение 2.0 * sigma * sigma, используется для ускорения вычислений
        double m_factor = 1.0;   ///< Предрасчитанное значение 1.0 / ( sqrt ( 2.0 * CV_PI ) * sigma * sigma * sigma ), используется для ускорения вычислений
        double m_span = 10e+7;   ///< "Размах" отклика функции ортогонально направлению перепада
    }; // GaussDirecterDerivativeTemplate

    template < class CoordType >
    double first_directed_derivative::operator () ( const Point_ < CoordType > &at ) const
    {
        return operator () ( at.x, at.y );
    }

    template < class CoordType >
    double first_directed_derivative::operator () ( const CoordType x, const CoordType y ) const
    {
        const double r = x * m_cos + y * m_sin;
        const double span_r = -x * m_sin + y * m_cos;
        return std::abs ( span_r ) < m_span ? r * m_factor * exp ( - r * r / m_2sigma2 ) : 0.0;
    }

    /**
    * \brief	Вторая направленная производная, используется как функтор для создания маски 
    */
    class second_directed_derivative
    {
    public:
        /**
        * \brief	Создать на основе угла поворота в плоскости angle_rad (радианы) и размаха sigma
        */
        second_directed_derivative ( const double angle_rad = 0.0, const double sigma = 1.0, const double span = 10e+7 );

        /**
        * \brief	Расчитатить значение функции в заданной точке
        * \tparam 	CoordType   Тип координат для расчета значения. Позволяет использовать как дискретные, так и непрерывные сетки
        * \param 	at          Положение интересующей точки
        * \return 	Значение производной в заданной параметром точке
        */
        template < class CoordType >
        double operator () ( const Point_ < CoordType > &at ) const;

        /**
        * \brief	Расчитатить значение функции в заданной двумя координатами точке
        * \tparam 	CoordType   Тип координат для расчета значения. Позволяет использовать как дискретные, так и непрерывные сетки
        * \param 	x           Горизонтальное смещение точки
        * \param 	у           Вертикальное смещение точки
        * \return 	Значение производной в заданной параметром точке
        */
        template < class CoordType >
        double operator () ( const CoordType x, const CoordType y ) const;

    private:
        double m_sin = 0.0,  ///< Синус заданного при создании функции угла, используется для ускорения вычислений
               m_cos = 0.0;  ///< Косинус заданного при создании функции угла, используется для ускорения вычислений

        double m_2sigma2 = 2.0;  ///< Предрасчитанное значение 2.0 * sigma * sigma, используется для ускорения вычислений
        double m_factor = 1.0;   ///< Предрасчитанное значение 1.0 / ( sqrt ( 2.0 * CV_PI ) * sigma * sigma * sigma ), используется для ускорения вычислений
        double m_sigma2 = 2.0;   ///< Предрасчитанное значение sigma * sigma, используется для ускорения вычислений
        double m_span = 10e+7;   ///< "Размах" отклика функции ортогонально направлению перепада
    }; // GaussDirecterDerivativeTemplate

    template < class CoordType >
    double second_directed_derivative::operator () ( const Point_ < CoordType > &at ) const
    {
        return operator () ( at.x, at.y );
    }

    template < class CoordType >
    double second_directed_derivative::operator () ( const CoordType x, const CoordType y ) const
    {
        const double r = x * m_cos + y * m_sin,
                     r2 = r * r;
        const double span_r = -x * m_sin + y * m_cos;
        return std::abs ( span_r ) < m_span ? m_factor * ( 1.0 - r2 / m_sigma2 ) * exp ( -r2 / m_2sigma2 ) : 0.0;
    }

}; // namespace GaussFunction
