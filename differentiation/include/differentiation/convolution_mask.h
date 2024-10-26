#pragma once

/*!
    \file
    \brief Маска свертки для хранения и использования откликов аналитически заданных функций
    \author Мирошниченко С.Ю.
    \version 1.0

    Класс расчитан на разряженные маски свертки, в которых ненулевых элементов не более 30%. 
*/

#include <list>
#include <fstream>

#include <cmath>
#include <opencv2/opencv.hpp>

using namespace cv;

template < class DataType >
class convolution_mask_t
{
public:

    /**
     * \brief	Элемент разреженного представления маски свертки
     */
    struct Item
    {
        /**
         * \brief	Значение элемента
         */
        DataType value;

        /**
         * \brief	Положение в маске. Координата симметрична относительно нуля по обеим осям
         */
        Point2i at;
    };

    /**
    * \brief	Представление маски в виде списка элементов, хорошо подходит для стандарных алгормтов за счет одномерности
    */
    using items = std::list < Item >;

    /**
    * \brief	Создать пустую маску
    */
    convolution_mask_t ();

    /**
    * \brief	Создать произвольную маску на основе порождающей функции (например Гаусса, Лапласиана и т.п.)
    * \param 	function   	Порождающая функция - вызываемый объект с двумя параметрами, соответсвующими горизонтальной в вертикальной координатам рассчитываемого элемента
    * \param 	halfWidth   Полуширина маски по горизонтали (для маски размером 3х3 полуширина - 1 в обоих измерениях)
    * \param 	halfHeight  Полувысота маски по горизонтали
    * \param 	zeroThreshold   Нулевой порог, менее которого рассчитанное значение приводится к 0
    */
    template < class Function >
    convolution_mask_t ( const Function &function, const int halfWidth, const int halfHeight, const double zeroThreshold = 1e-4 );

    /**
    * \brief	Создать квадратную маску на основе порождающей функции (например Гаусса, Лапласиана и т.п.)
    * \param 	function   	Порождающая функция - вызываемый объект с двумя параметрами, соответсвующими горизонтальной в вертикальной координатам рассчитываемого элемента
    * \param 	halfSize    Полуразмер (для маски размером 3х3 полуразмер - 1 в обоих измерениях)
    * \param 	zeroThreshold   Нулевой порог, менее которого рассчитанное значение приводится к 0
    */
    template < class Function >
    convolution_mask_t ( const Function &function, const int halfSize, const double zeroThreshold = 1e-4 );

    /**
    * \brief	STL-совместимая функция для возврата итератора начала последовательности элементов маски
    */
    auto begin () const;

    /**
    * \brief	STL-совместимая функция для возврата итератора конца последовательности элементов маски
    */
    auto end () const;

    /**
    * \brief	Масштабный коэффициент, используемый функцией свертки для приведения расчитанного отклика к заданному диапазону значений. Расчитывается автоматически при создании маски
    */
    const DataType scale_factor () const;

    /**
    * \brief	Сдвиг (аддитивный) значения отклика используемый для его центрирования в нуле. Расчитывается автоматически при создании маски
    */
    const double shift () const;

    /**
    * \brief	Вычисление значения отклика путем свертки маски с матрицей what с центром в точке at. Размер области светки зависит от матрицы
    */
    template < class MatType >
    const DataType operator () ( const Mat &what, const Point2i &at ) const;

    /**
    * \brief	Вычисление значения отклика путем свертки маски с матрицей what с центром в точке (x, y). Размер области светки зависит от матрицы
    */
    template < class MatType >
    const DataType operator () ( const Mat &what, const int x, const int y ) const;

    /**
    * \brief	Свертка всей матрицы what с текущей маской.
    * \tparam 	MatType     Тип данных входной матрицы, должен соответствовать what.type ()
    * \tparam 	OutMatType  Тип данных выходной матрицы, должен соответствовать outMatType
    * \param 	what        Исходная матрица для свертки
    * \param 	mask        Бинарная маска, нулевые значения элементов которой блокируют расчет значений
    * \param 	out_mat_type  Тип данных результирующей матрицы (один из типов данных OpenCV)
    * \return   Матрица откликов для текущей маски. Размер матрицы соотвествует размеру входной. 
                Для того, чтобы не обрабатывать особым образом края кадра периметр матрицы шириной 50 точек не обработывается.
    */
    template < class MatType, class OutMatType, class MaskType = uint8_t >
    Mat conv ( const Mat &what, const int out_mat_type, const Mat &mask = {} ) const;

    /**
    * \brief	Свертка матрицы what с текущей маской с заданным шагом step по горизонтали и вертикали (разреженная свертка).
    * \tparam 	MatType     Тип данных входной матрицы, должен соответствовать what.type ()
    * \tparam 	OutMatType  Тип данных выходной матрицы, должен соответствовать outMatType
    * \param 	what        Исходная матрица для свертки
    * \param 	outMatType  Тип данных результирующей матрицы (один из типов данных OpenCV)
    * \param 	step        Шаг по исходной матрице по горизонтали и вертикали
    * \return   Матрица откликов для текущей маски. Размер матрицы соотвествует размеру входной. 
                Для того, чтобы не обрабатывать особым образом края кадра периметр матрицы шириной 50 точек не обработывается.
    */
    template < class MatType, class OutMatType >
    Mat sparsed_conv ( const Mat &what, const int outMatType, const Point2i &step ) const;

    /**
    * \brief	Общий размер маски
    */
    Size size () const;

    /**
    * \brief	Полуразмер маски (исключая центральный элемент)
    */
    Size semi_size () const;

private:
    /**
    * \brief	Список элементов маски
    */
    items m_items;

    /**
    * \brief	Масштабный коэффициент
    */
    DataType m_scale;

    /**
    * \brief	Сдвиг (аддитивный) значения отклика используемый для его центрирования в нуле. Расчитывается автоматически при создании маски
    */
    double m_shift;

    /**
    * \brief	Полуширина маски
    */
    const int m_half_width;

    /**
    * \brief	Полувысота маски
    */
    const int m_half_height;
};

/**
* \brief	Для повышения производительности используются целочисленные маски
*/
using convolution_mask = convolution_mask_t < int >;

template < class StreamType, class DataType >
StreamType & operator << ( StreamType &stream, const convolution_mask_t < DataType > &mask );

#include "convolution_mask.hpp"
