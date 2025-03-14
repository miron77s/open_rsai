# Инструкция (туториал) по работе с библиотекой Open Remote Sensing AI Library (OpenRSAI)

Данная инструкция описывает процесс сборки, установки, настройки и использования библиотеки под управлением ОС Ubuntu Linux 22.04.

## Установка необходимых драйверов и библиотек

Сборка и использование библиотеки требует наличия драйвера графического ускорителя NVIDIA, инструментария CUDA и библиотеки CUDNN. Развернутое описание их установки установки приведено в [руководстве по подготовке ОС Ubuntu 22.04](https://github.com/miron77s/open_rsai/blob/main/tutorial/REQUIREMENTS.md).    

## Минимальные технические требования

Минимальные технические требования.
Для запуска и использования всех компонентов открытой библиотеки обработки данных дистанционного зондирования Земли OpenRSAI необходима следующая аппаратная конфигурация:
- 16Гб ОЗУ;
- графический ускоритель NVIDIA GeForce 4090 (24Гб видеопамяти);
- суммарно 50Гб свободного дискового пространства для размещения необходимых для сборки, установки и обучения данных.


## Сборка и установка библиотеки

Общий процесс сборки и установки компонентов библиотеки включает следующие этапы:

1. Установить зависимости ядра библиотеки OpenRSAI-Core: [пакеты операционной системы](https://github.com/miron77s/open_rsai#system-packages) и [Segment Anything для Python](https://github.com/miron77s/open_rsai#segment-anything).
2. Установить зависимости библиотеки алгоритмов [OpenRSAI-Algos](https://github.com/miron77s/open_rsai_algos#installation) и конвейера подготовки обучающей выборки [OpenRSAI-Markup](https://github.com/miron77s/open_rsai_markup?tab=readme-ov-file#requirements).
3. В домашнем каталоге пользователя создать каталог `open_rsai_project` и подкаталог `src` для размещения исходных кодов:

```
mkdir open_rsai_project
cd open_rsai_project
mkdir src
cd src
```

4. Клонировать репозиторий алгоритмов библиотеки [OpenRSAI-Algos](https://github.com/miron77s/open_rsai_algos), загрузить субмодули:

```
git clone https://github.com/miron77s/open_rsai_algos
cd open_rsai_algos
git submodule update --init --recursive
cd  ..
```

5. Клонировать репозиторий инструментария для подготовки обучающей выборки [OpenRSAI-Markup](https://github.com/miron77s/open_rsai_markup):

```
git clone https://github.com/miron77s/open_rsai_markup
```

6. При необходимости установить `rsync` и перенести содержимое `open_rsai_markup` в каталог `open_rsai_algos`:

```
sudo apt -y install rsync
```

```
rsync -av --exclude='.git' open_rsai_markup/ open_rsai_algos/
rm -rf open_rsai_markup
```

7. Собрать библиотеку и инструментарий:

```
cd open_rsai_algos
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=../../distr -DENABLE_INSTALLER=ON
cmake --build build --parallel ${nproc}
cmake --install build
cd ..
```

8. Установить алгоритмы детектирования объектов местности:

```
conda activate open_rsai_detectors
cd open_rsai_algos/mrcnn_library
python setup.py install
conda deactivate
```

9. Установить QGIS согласно [инструкции](https://qgis.org/en/site/forusers/alldownloads.html#debian-ubuntu), после чего ОБЯЗАТЕЛЬНО запустить QGIS (при первом запуске инициализируется каталог плагинов).

10. Клонировать репозиторий инструментария OpenRSAI-QGIS и установить плагины:

```
git clone https://github.com/miron77s/open_rsai_qgis_plugins
cd open_rsai_qgis_plugins
sh install.sh
cd ..
```

11. Добавить установленные плагины в инструментарий QGIS согласно [инструкции](https://github.com/miron77s/open_rsai_qgis_plugins#4-activating-plugins-in-qgis).

12. Установить необходимые пакеты операционной системы для распаковки обучающих данных согласно [инструкции OpenRSAI-Data](https://github.com/miron77s/open_rsai_data#requirements). 

13. Клонировать репозиторий обучающих данных OpenRSAI-Data, загрузить данные для обучения и тестирования инструментов библиотеки:

```
cd ../distr
git clone https://github.com/miron77s/open_rsai_data data
cd data
sh download_all.sh
```

## Обучение и тестирование детектора зданий и сооружений

1. Создать обучающую выборку для детектирования зданий и сооружений:

```
cd ../bin
./markup_writer -r ../data/raster/Kursk/Bing_19_3395.tif,../data/raster/Kursk/Google_19_3395.tif,../data/raster/Kursk/Mapbox_19_3395.tif,../data/raster/Kursk/Yandex_19_3395.tif -v ../data/buildings/vector/Kursk/eastern_industrial_en/,../data/buildings/vector/Kursk/eastern_industrial_es/,../data/buildings/vector/Kursk/eastern_industrial_w/,../data/buildings/vector/Kursk/eastern_region/,../data/buildings/vector/Kursk/k3t3_e/,../data/buildings/vector/Kursk/k3t3_w,../data/buildings/vector/Kursk/klykova_plevitskaya/ -o ../data/buildings/markup_roofs/ -b 0.4 -c roofs -d ../data/buildings/vector/forbidden/forbidden.shp --format yolo -m replace -t 608x608
```

2. Обучить детектор зданий и сооружений:

```
./darknet detector train ../data/buildings/markup_roofs/rsai.data ./weights/yolov4-buildings-15k.cfg ./weights/yolov4.conv.137
```
Обучение является длительным (не менее 9 часов на GeForce 4090), необходимо дождаться его окончания.

В комплекте библиотеки присутствуют предобученные веса детекатора зданий и сооружений, поэтому данный этап является необязательным.

3. Протестировать точность (IoU) детектирования зданий и сооружений на валидационной выборке, на которой сеть не обучалась.

Для весов, полученных в результате обучения:

```
./darknet detector map ../data/buildings/markup_roofs/rsai.data ./weights/yolov4-buildings-15k.cfg ../data/buildings/markup_roofs/backup/yolov4-buildings-15k_final.weights
```

Для предобученных весов из состава библиотеки:

```
./darknet detector map ../data/buildings/markup_roofs/rsai.data ./weights/yolov4-buildings-15k.cfg ./weights/yolov4-buildings-15k_final.weights
```

4. Протестировать детектирование зданий и сооружений на отдельно взятом валидационном примере:

Для весов, полученных в результате обучения:

```
./darknet detector test ../data/buildings/markup_roofs/rsai.data ./weights/yolov4-buildings-15k.cfg ../data/buildings/markup_roofs/backup/yolov4-buildings-15k_final.weights ../data/buildings/markup_roofs/valid/Kursk_Bing_19_3395_7914.jpg 
```

Для предобученных весов из состава библиотеки:

```
./darknet detector test ../data/buildings/markup_roofs/rsai.data ./weights/yolov4-buildings-15k.cfg ./weights/yolov4-buildings-15k_final.weights ../data/buildings/markup_roofs/valid/Kursk_Bing_19_3395_7914.jpg 
```

Важно. Нумерация изображения в сгенерированной выборке может отличаться, поэтому в случае отсутсвия снимка `Kursk_Bing_19_3395_7914.jpg` следует использовать любой другой из каталога `../data/buildings/markup_roofs/valid/` с непустым файлом аннотации `Kursk_Bing_19_3395_NNNN.txt` (значит на изображении должны присутствовать зданий или сооружения).

## Автоматическое построение карт зданий и сооружений

1. Активировать окружение conda `open_rsai_core_sam`:

```
conda activate open_rsai_core_sam
```

2. Выполнить подготовку рабочей области для каждого из используемых для детектирования растров:

```
./raster_inliers_extractor -r ../data/raster/Kursk/Bing_19_3395.tif -v ../data/buildings/vector/osm/inliers.shp -i ../data/buildings/vector/Kursk/western_1/roi.shp -o ../data/buildings/vector/Kursk/western_1/ -f
./objects_bounds_finder -r ../data/raster/Kursk/Bing_19_3395.tif -v ../data/buildings/vector/Kursk/western_1/inliers.shp --metadata ../data/buildings/vector/Kursk/western_1/Bing_19_3395/meta.shp -o ../data/buildings/vector/Kursk/western_1/Bing_19_3395/ --mask 50 --max_proj_length 80 -f
./objects_bounds_finder -r ../data/raster/Kursk/Google_19_3395.tif -v ../data/buildings/vector/Kursk/western_1/inliers.shp --metadata ../data/buildings/vector/Kursk/western_1/Google_19_3395/meta.shp -o ../data/buildings/vector/Kursk/western_1/Google_19_3395/ --mask 50 --max_proj_length 80 -f
./objects_bounds_finder -r ../data/raster/Kursk/Mapbox_19_3395.tif -v ../data/buildings/vector/Kursk/western_1/inliers.shp --metadata ../data/buildings/vector/Kursk/western_1/Mapbox_19_3395/meta.shp -o ../data/buildings/vector/Kursk/western_1/Mapbox_19_3395/ --mask 50 --max_proj_length 80 -f
./objects_bounds_finder -r ../data/raster/Kursk/Yandex_19_3395.tif -v ../data/buildings/vector/Kursk/western_1/inliers.shp --metadata ../data/buildings/vector/Kursk/western_1/Yandex_19_3395/meta.shp -o ../data/buildings/vector/Kursk/western_1/Yandex_19_3395/ --mask 50 --max_proj_length 80 -f
```

3. Выполнить детектирование крыш зданий на снимках:

```
./roof_locator -r ../data/raster/Kursk/Bing_19_3395.tif -v ../data/buildings/vector/Kursk/western_1/Bing_19_3395/bounds.shp -o ../data/buildings/vector/Kursk/western_1/Bing_19_3395/ -f --use_sam
./roof_locator -r ../data/raster/Kursk/Google_19_3395.tif -v ../data/buildings/vector/Kursk/western_1/Google_19_3395/bounds.shp -o ../data/buildings/vector/Kursk/western_1/Google_19_3395/ -f --use_sam
./roof_locator -r ../data/raster/Kursk/Mapbox_19_3395.tif -v ../data/buildings/vector/Kursk/western_1/Mapbox_19_3395/bounds.shp -o ../data/buildings/vector/Kursk/western_1/Mapbox_19_3395/ -f --use_sam
./roof_locator -r ../data/raster/Kursk/Yandex_19_3395.tif -v ../data/buildings/vector/Kursk/western_1/Yandex_19_3395/bounds.shp -o ../data/buildings/vector/Kursk/western_1/Yandex_19_3395/ -f --use_sam
```

4. Провести мультипроекционную реконструкцию зданий и сооружений (с комплексированием):

```
./multiview_building_reconstructor -r ../data/raster/Kursk/Bing_19_3395.tif,../data/raster/Kursk/Google_19_3395.tif,../data/raster/Kursk/Mapbox_19_3395.tif,../data/raster/Kursk/Yandex_19_3395.tif -v ../data/buildings/vector/Kursk/western_1/Bing_19_3395/roofs.shp,../data/buildings/vector/Kursk/western_1/Google_19_3395/roofs.shp,../data/buildings/vector/Kursk/western_1/Mapbox_19_3395/roofs.shp,../data/buildings/vector/Kursk/western_1/Yandex_19_3395/roofs.shp -b ../data/buildings/vector/Kursk/western_1/Bing_19_3395/bounds.shp,../data/buildings/vector/Kursk/western_1/Google_19_3395/bounds.shp,../data/buildings/vector/Kursk/western_1/Mapbox_19_3395/bounds.shp,../data/buildings/vector/Kursk/western_1/Yandex_19_3395/bounds.shp -o ../data/buildings/vector/Kursk/western_1/Bing_19_3395/,../data/buildings/vector/Kursk/western_1/Google_19_3395/,../data/buildings/vector/Kursk/western_1/Mapbox_19_3395/,../data/buildings/vector/Kursk/western_1/Yandex_19_3395/ -f --height_factor 0.4
```

5. Проверить результаты детектирования и реконструкции в QGIS, последовательно выполняя следующие команды:

```
qgis --code ../data/buildings/vector/Kursk/western_1/Bing_19_3395/open_in_qgis.py
```
```
qgis --code ../data/buildings/vector/Kursk/western_1/Google_19_3395/open_in_qgis.py
```
```
qgis --code ../data/buildings/vector/Kursk/western_1/Mapbox_19_3395/open_in_qgis.py
```
```
qgis --code ../data/buildings/vector/Kursk/western_1/Yandex_19_3395/open_in_qgis.py
```
В процессе проверки убедиться в наличии значений высот объектов (поле "height") в слоях "projes" и "shades".

6. Отключить окружение conda:

```
conda deactivate
```

## Создание мультивременных композитных изображений

1. Создать растровый мультивременной композит в синем цветовом канале и отобразить его в QGIS:

```
sh ./scripts/raster_composer.sh "../data/raster/Kursk/Mapbox_19_3395.tif" "../data/raster/Kursk/Bing_19_3395.tif" "../data/composite/" "4020549.6 6718068.8 4023724.3 6715727.0" "raster_composite" blue
qgis ../data/composite/raster_composite_blue.tif 
```

2. Создать растровый мультивременной композит в красном цветовом канале и отобразить его в QGIS:

```
sh ./scripts/raster_composer.sh "../data/raster/Kursk/Mapbox_19_3395.tif" "../data/raster/Kursk/Bing_19_3395.tif" "../data/composite/" "4020549.6 6718068.8 4023724.3 6715727.0" "raster_composite" red
qgis ../data/composite/raster_composite_red.tif 
```

3. Создать векторное композитное изображение с каналом прозрачности поверх растра:

```
sh ./scripts/vector_composer.sh ../data/buildings/vector/Kursk/eastern_industrial_w/updater.shp ../data/buildings/vector/Kursk/eastern_industrial_w/updating.shp ../data/composite/ vector_composite "4033053.7 6720432.3 4034968.2 6718771.9"
qgis ../data/raster/Kursk/Bing_19_3395.tif ../data/composite/vector_composite.tif
``` 

4. Создать и отобразить растр-векторное мультивременное композитное изображение:

```
sh ./scripts/vector_composer.sh ../data/buildings/vector/Kursk/eastern_industrial_w/updater.shp ../data/buildings/vector/Kursk/eastern_industrial_w/updating.shp ../data/composite/ vector_composite_on_raster "4033053.7 6720432.3 4034968.2 6718771.9" ../data/raster/Kursk/Bing_19_3395.tif
qgis ../data/composite/vector_composite_on_raster.tif
```
## Cопоставление детектированных объектов с опорной цифровой картой

Выполнить и визуализировать результаты сопоставления детектированных объектов `updater.shp` c опорными данными `updating.shp` согласно маске сопоставления `update_mask.shp`:

```
./map_updater -v ../data/buildings/vector/Kursk/eastern_industrial_w/updater.shp -u ../data/buildings/vector/Kursk/eastern_industrial_w/updating.shp -i ../data/buildings/vector/Kursk/eastern_industrial_w/update_mask.shp -o ../data/detect_differences/ --save_diff --save_updated
qgis ../data/detect_differences/upcomming.shp ../data/detect_differences/outdated.shp
```

В результате формируется 2 карты: 
- карта новых объектов `upcomming.shp`, не присутствовавших на опорной карте;
- карта устаревших объектов `outdated.shp`.

## Обучение и тестирование детектора гидрографии

1. Активировать окружение `open_rsai_detectors`

```
conda activate open_rsai_detectors
```

2. Запустить скрипт обучения детектора 

```
python ./train_scripts/train_hydro.py ../data/hydro/markup/water_all/ ./weights/mask_rcnn_coco.h5
```
Весовые коэффициенты при обучении будут сохранены в каталог "hydro`yyyyMMdd`T`HHmm`", где yyyy-MM-dd HH:mm - дата и время начала обучения.

Важно. Сохранение весовых коэффициентов для последующего тестирования требует 40Гб дискового пространства.

Процесс обучения на GPU NVIDIA GeForce 4090 занимает 7-8 часов.

3. Выполнить тестирование сохраненных весовых коэффициентов 

```
python ./train_scripts/test_hydro.py ../data/hydro/markup/water_all/ ./"hydro20241024T0830/" 1 150 not_save
```

Вместо имени каталога `hydro20241024T0830` необходимо подставить имя каталога весов, созданного в результате обучения.

Важно. Процесс тестирования на GPU NVIDIA GeForce 4090 занимает ~2 часа.

4. Деактивировать окружение `open_rsai_detectors`

```
conda deactivate
```

## Построение карт гидрографии

```
conda activate open_rsai_detectors
```

2. Запустить скрипт детекторования гидрографии в заданной области `../data/hydro/vector/roi/roi.shp` снимка `../data/raster/Kursk/Bing_19_3395.tif` для предустановленной модели:

```
python ./detect_hydro.py ../data/hydro/ ./weights/mask_rcnn_hydro_0115.h5 ../data/hydro/vector/roi/roi.shp ../data/raster/Kursk/Bing_19_3395.tif
```

или для созданной на этапе обучения 
```
python ./detect_hydro.py ../data/hydro/ ./"hydro20241024T0830/mask_rcnn_hydro_0150.h5 ../data/hydro/vector/roi/roi.shp ../data/raster/Kursk/Bing_19_3395.tif
```
где вместо имени каталога `hydro20241024T0830` необходимо подставить имя каталога весов, созданного в результате обучения, а также выбрать файл модели `mask_rcnn_hydro_****.h5` соответствующий лучшей по статистике тестирования эпохе.

Важно. Результат будет сохранен в слое `hydro` каталога `../data/hydro/`. 

3. Деактивировать окружение `open_rsai_detectors`

```
conda deactivate
```

4. Визуализировать результаты детектирования 

```
qgis ../data/raster/Kursk/Bing_19_3395.tif ../data/hydro/hydro.shp
```

## Обучение и тестирование детектора растительности

1. Активировать окружение `open_rsai_detectors`

```
conda activate open_rsai_detectors
```

2. Запустить скрипт обучения детектора.  

```
python ./train_scripts/train_green.py ../data/green/markup/vegetation_all/ ./weights/mask_rcnn_coco.h5
```

Весовые коэффициенты при обучении будут сохранены в каталог "green`yyyyMMdd`T`HHmm`", где yyyy-MM-dd HH:mm - дата и время начала обучения.

Важно. Сохранение весовых коэффициентов для последующего тестирования требует 40Гб дискового пространства.
Процесс обучения на GPU NVIDIA GeForce 4090 занимает ~7-8 часов.

3. Выполнить тестирование сохраненных весовых коэффициентов 

```
python ./train_scripts/test_green.py ../data/green/markup/vegetation_all/ ./"green20241024T0830/" 1 150 not_save
```

Вместо имени каталога `green20241024T0830` необходимо подставить имя каталога весов, созданного в результате обучения.

Важно. Процесс тестирования на GPU NVIDIA GeForce 4090 занимает ~2 часа.

4. Деактивировать окружение `open_rsai_detectors`

```
conda deactivate
```

## Построение карт растительности

```
conda activate open_rsai_detectors
```

2. Запустить скрипт детекторования гидрографии в заданной области `../data/green/vector/roi/roi.shp` снимка `../data/raster/Kursk/Bing_19_3395.tif` для предустановленной модели:

```
python ./detect_green.py ../data/green/ ./weights/mask_rcnn_green_0073.h5 ../data/green/vector/roi/roi.shp ../data/raster/Kursk/Bing_19_3395.tif
```
или для созданной на этапе обучения:
```
python ./detect_green.py ../data/green/ ./"green20241024T0830/mask_rcnn_green_0150.h5 ../data/green/vector/roi/roi.shp ../data/raster/Kursk/Bing_19_3395.tif
```
где вместо имени каталога `green20241024T0830` необходимо подставить имя каталога весов, созданного в результате обучения, а также выбрать файл модели `mask_rcnn_green_****.h5` соответствующий лучшей по статистике тестирования эпохе.

Важно. Результат будет сохранен в слое `green` каталога `../data/green/`. 

3. Деактивировать окружение `open_rsai_detectors`

```
conda deactivate
```

4. Визуализировать результаты детектирования 

```
qgis ../data/raster/Kursk/Bing_19_3395.tif ../data/green/green.shp
```