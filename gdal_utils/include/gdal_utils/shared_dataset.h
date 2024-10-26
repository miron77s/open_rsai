#pragma once

/*!
\file
\authors miron77s
\version 1.0.0

Файл содержит классы умных указателей для наборов данных библиотеки GDAL
*/

#include <memory>
#include <gdal_priv.h>
#include <string>
#include <vector>
#include "shared_spatial_reference.h"

namespace gdal
{
    using ogr_layer = OGRLayer;

	/// Класс умного указателя для набора данных библиотеки GDAL
	/*!
		Реализован на основе std::shared_ptr, удаляет хранимый набор с помощью GDALClose
	*/
	class shared_dataset:
		public std::shared_ptr < GDALDataset >
	{
	public:
		/*!
			Создание объекта на основе пустого указателя (nullptr)
		*/
        shared_dataset ();

		/*!
			Создание объекта на основе ранее открытого набора данных
			\param[in] p_ds Набор данных созданный с помощью GDALOpen/GDALOpenEx/GDALDriver::Create/GDALDriver::CreateCopy
		*/
        shared_dataset ( GDALDataset * p_ds );

        /*!
            Чтение растровой системы координат. Обертка для GetProjectionRef с созданием объекта OGRSpatialReference для версий GDAL < 3.0
            \return действительный указатель OGRSpatialReference в виде shared_spatial_reference в случае успеха / shared_spatial_reference ( nullptr ) - в случае неудачи
        */
        shared_spatial_reference    raster_srs          () const;

        /*!
            Чтение векторной системы координат. Обертка для GetSpatialRef с проверкой наличия слоя с заданным номером
            \param[in] layer        Номер векторного слоя для получения системы координат
            \return действительный указатель OGRSpatialReference в виде shared_spatial_reference в случае успеха / shared_spatial_reference ( nullptr ) - при отсуствии требуемого слоя
        */
        shared_spatial_reference    vector_srs          ( int layer ) const;

        /*!
            Поиск слоя с заданным именем и возврат его индекса (оказалось, что стандартного решения в GDAL нет)
            \param[in] layer_name   Название искомого слоя
            \return номер слоя или -1, если таковой не найден
        */
        int                         layer_index_by_name ( const std::string &layer_name ) const;
	}; // class shared_dataset

	/*!
		Обертка для функции GDALOpenEx
		\warning Перед первым вызовом функции автоматически выполняется инициализация с помощью GDALRegisterGTiff
        \param[in] file_name        Имя файла для создания набора данных GDAL
        \param[in] mode             Флаги режима открытия GDAL_OF_RASTER, GDAL_OF_READONLY, GDAL_OF_READWRITE и др.
        \param[in] allowed_drivers  Перечень ключений драйверов для открытия, nullptr - для перебора всех зарестрированных драйверов
        \param[in] open_options     Список опций открытия
        \param[in] sibling_files    Список дочерных файлов
		\return действительный указатель GDALDataset в виде shared_dataset в случае успеха / shared_dataset ( nullptr ) - в случае неудачи
	*/
    shared_dataset open_dataset ( const std::string & file_name, int mode, const char * const * allowed_drivers = nullptr,
                                  const char * const * open_options = nullptr, const char * const * sibling_files = nullptr );

	/*!
		Обертка для функции GDALOpenEx для открытия вложенного набора данных путем чтения метаданных из секции "SUBDATASETS"
		\warning Перед первым вызовом функции автоматически выполняется инициализация с помощью GDALRegisterGTiff
        \param[in] file_name        Имя файла для создания набора данных GDAL
		\param[in] subdataset_index Индекс вложенного набора (нумерация с 1)
        \param[in] mode             Флаги режима открытия GDAL_OF_RASTER, GDAL_OF_READONLY, GDAL_OF_READWRITE и др.
        \param[in] allowed_drivers  Перечень ключений драйверов для открытия, nullptr - для перебора всех зарестрированных драйверов
        \param[in] open_options     Список опций открытия
        \param[in] sibling_files    Список дочерных файлов
		\return действительный указатель GDALDataset в виде shared_dataset в случае успеха / shared_dataset ( nullptr ) - в случае неудачи
	*/
    shared_dataset open_subdataset ( const std::string & file_name, int subdataset_index, int mode, const char * const * allowed_drivers = nullptr,
                                     const char * const * open_options = nullptr, const char * const * sibling_files = nullptr );

    /*!
        Обертка для функции GDALDriver::Create
        \warning Перед первым вызовом функции автоматически выполняется инициализация с помощью GDALRegisterGTiff
        \param[in] driver_name      Строковое ммя драйвера GDAL для создания набора данных
        \param[in] file_name        Имя файла для создания набора данных GDAL
        \param[in] width            Количество столбцов / горизонтальный размер (только для растров)
        \param[in] height           Количество строк / вертикальный размер (только для растров)
        \param[in] bands            Количество каналов / цветовых плоскостей (только для растров)
        \param[in] type             Тип точки растра (только для растров)
        \return действительный указатель GDALDataset в виде shared_dataset в случае успеха / shared_dataset ( nullptr ) - в случае неудачи
    */
    shared_dataset create_dataset ( const std::string &driver_name, const std::string & file_name, int width = 0, int height = 0, int bands = 0, GDALDataType type = GDT_Unknown );

    using shared_datasets = std::vector < shared_dataset >;

    bool operator ! ( const shared_datasets &datasets );

}; // namespace gdal
