# Инструкция (туториал) по работе с библиотекой Open Remote Sensing AI Library (OpenRSAI)

Данная инструкция описывает процесс сборки, установки, настройки и использования библиотеки под управлением ОС Ubuntu Linux 22.04.

## Сборка и установка

Общий процесс сборки и установки компонентов библиотеки включает следующие этапы:

1. Установить зависимости ядра библиотеки OpenRSAI-Core: [пакеты операционной системы](https://github.com/miron77s/open_rsai#system-packages) и [Segment Anything для Python](https://github.com/miron77s/open_rsai#segment-anything).
2. Установить [зависимости](https://github.com/miron77s/open_rsai_algos#installation) библиотеки алгоритмов OpenRSAI-Algos.
3. В домашнем каталоге пользователя создать каталог `open_rsai` и подкаталог `src` для размещения исходных кодов:

```
mkdir open_rsai
cd open_rsai
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

6. Перенести содержимое `open_rsai_markup` в каталог `open_rsai_algos`:

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

8. Установить QGIS согласно [инструкции](https://qgis.org/en/site/forusers/alldownloads.html#debian-ubuntu).

9. Клонировать репозиторий инструментария OpenRSAI-QGIS и установить плагины:

```
git clone https://github.com/miron77s/open_rsai_qgis_plugins
cd open_rsai_qgis_plugins
sh install.sh
cd ..
```

10. Добавить установленные плагины в инструментарий QGIS согласно [инстурукции](https://github.com/miron77s/open_rsai_qgis_plugins#4-activating-plugins-in-qgis).

11. Установить необходимые пакеты операционной системы для распаковки обучающих данных согласно [инструкции OpenRSAI-Data](https://github.com/miron77s/open_rsai_data#requirements). 

12. Клонировать репозиторий обучающих данных OpenRSAI-Data, загрузить данные для обучения и тестирования инструментов библиотеки:

```
cd ../distr
git clone https://github.com/miron77s/open_rsai_data data
cd data
sh download_all.sh
```

## Обучение и тестирование детектора зданий и сооружений

1. Создать обучающую выборку для детектирования крыш зданий и сооружений:

```
cd ../bin
./markup_writer -r ../data/raster/Kursk/Bing_19_3395.tif,../data/raster/Kursk/Google_19_3395.tif,../data/raster/Kursk/Mapbox_19_3395.tif,../data/raster/Kursk/Yandex_19_3395.tif -v ../data/buildings/vector/Kursk/eastern_industrial_en/,../data/buildings/vector/Kursk/eastern_industrial_es/,../data/buildings/vector/Kursk/eastern_industrial_w/,../data/buildings/vector/Kursk/eastern_region/,../data/buildings/vector/Kursk/k3t3_e/,../data/buildings/vector/Kursk/k3t3_w,../data/buildings/vector/Kursk/klykova_plevitskaya/ -o ../data/buildings/markup_roofs/ -b 0.4 -c roofs -d ../data/buildings/vector/forbidden/forbidden.shp --format yolo -m replace -t 608x608
```

2. Обучить детектор крыш зданий и сооружений:

```
./darknet detector train ../data/buildings/markup_roofs/rsai.data ./weights/yolov4-buildings-15k.cfg ./weights/yolov4.conv.137
```
Обучение является длительным (не менее 9 часов на GeForce 4090), необходимо дождаться его окончания.

В комплекте библиотеки присутствуют предобученные веса детекатора зданий и сооружений, поэтому данный этап является необязательным.

3. Протестировать точность (IoU) детектирования крыш зданий и сооружений на валидационной выборке, на которой сеть не обучалась.

Для весов, полученных в результате обучения:

```
./darknet detector map ../data/buildings/markup_roofs/rsai.data ./weights/yolov4-buildings-15k.cfg ../data/buildings/markup_roofs/backup/yolov4-buildings-15k_final.weights
```

Для предобученных весов из состава библиотеки:

```
./darknet detector map ../data/buildings/markup_roofs/rsai.data ./weights/yolov4-buildings-15k.cfg ./weights/yolov4-buildings-15k_final.weights
```

4. Протестировать детектирование крыш зданий и сооружений на отдельно взятом валидационном примере:

Для весов, полученных в результате обучения:

```
./darknet detector test ../data/buildings/markup_roofs/rsai.data ./weights/yolov4-buildings-15k.cfg ../data/buildings/markup_roofs/backup/yolov4-buildings-15k_final.weights ../data/buildings/markup_roofs/valid/Kursk_Bing_19_3395_7914.jpg 
```

Для предобученных весов из состава библиотеки:

```
./darknet detector test ../data/buildings/markup_roofs/rsai.data ./weights/yolov4-buildings-15k.cfg ./weights/yolov4-buildings-15k_final.weights ../data/buildings/markup_roofs/valid/Kursk_Bing_19_3395_7914.jpg 
```

Важно. Нумерация изображения в сгенерированной выборке может отличаться, поэтому в случае отсутсвия снимка `Kursk_Bing_19_3395_7914.jpg` следует использовать любой другой из каталога `../data/buildings/markup_roofs/valid/` с непустым файлом аннотации `Kursk_Bing_19_3395_NNNN.txt` (значит на изображении должны присутствовать зданий или сооружения).