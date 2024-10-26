#pragma once

/*!
\file
\authors miron77s
\version 1.0.0

Файл содержит классы умных указателей для представлений систем координат библиотеки GDAL
*/

#include <memory>
#include <ogr_spatialref.h>
#include <string>

namespace gdal
{
    /// Класс умного указателя для представления систем координат библиотеки GDAL
    /*!
        Реализован на основе std::shared_ptr, удаляет хранимый набор с помощью DestroySpatialReference
    */
    class shared_spatial_reference:
        public std::shared_ptr < OGRSpatialReference >
    {
    public:
        /*!
            Создание объекта на основе пустого указателя (nullptr)
        */
        shared_spatial_reference (): std::shared_ptr < OGRSpatialReference > ( nullptr, OGRSpatialReference::DestroySpatialReference ) {}

        /*!
            Создание объекта на основе ранее открытого набора данных
            \param[in] p_ref    Ранее созданная система координат
            \param[in] ds_name  Имя файла набора данных, которому принадлежит система координат
        */
        shared_spatial_reference ( OGRSpatialReference * p_ref, const std::string &ds_name = std::string () );

        /*!
            Представление в строковом виде. Обертка для exportToWkt
            \return представление системы координат в виде строки
        */
        std::string export_to_wkt           () const;

        /*!
            Представление в строковом виде с форматированием. Обертка для exportToPrettyWkt
            \return представление системы координат в виде строки с форматированием
        */
        std::string export_to_pretty_wkt    () const;

        /*!
            Имя файла набора данных, которому принадлежит система координат
            \return представление системы координат в виде строки с форматированием
        */
        std::string dataset_name            () const;

        /*!
            Проверка на наличие географической системы коодинат
            \return true в случае корректности
        */
        template < class outstream >
        bool        verify                  ( outstream &out ) const;

        static shared_spatial_reference from_epsg ( const int code );

    private:
        std::string _ds_name;

        template < class func, class string_type = std::string >
        string_type __to_std_string         ( const func &f ) const;

    }; // class shared_spatial_reference

    using shared_spatial_references = std::vector < shared_spatial_reference >;

    bool are_same ( const shared_spatial_references &srses );
    bool operator == ( const shared_spatial_references &lh, const shared_spatial_references &rh );
    bool operator == ( const shared_spatial_references &lh, const shared_spatial_reference &rh );
    bool operator != ( const shared_spatial_references &lh, const shared_spatial_references &rh );
    bool operator != ( const shared_spatial_references &lh, const shared_spatial_reference &rh );

}; // namespace gdal

#include "shared_spatial_reference.hpp"
