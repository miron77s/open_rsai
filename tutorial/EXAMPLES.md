# Сценарии работы с библиотекой

Перед первым запуском необходимо выполнить установку компонентов библиотеки согласно разделу инструкции ["Сборка и установка"](https://github.com/miron77s/open_rsai/blob/main/tutorial/TUTORIAL.md#%D1%81%D0%B1%D0%BE%D1%80%D0%BA%D0%B0-%D0%B8-%D1%83%D1%81%D1%82%D0%B0%D0%BD%D0%BE%D0%B2%D0%BA%D0%B0). 

## 0. Инструкция по запуску примеров

Для запуска любого из приведенных ниже примеров использования кода библиотеки необходимо выполнить следующие действия: 

- создать файл Python-скрипта (например, "example.py"), скопировать в него содержимое примера и сохранить в каталоге `open_rsai_project/distr/bin/` (чтобы использовать поставляемые с библиотекой примеры данных и файлы моделей);
- активировать окружение `open_rsai_detectors`;
- запустить созданный скрипт на исполнение, например:
```
python3 example.py
```
- дождаться завершения выполнения скрипта и сохранения результатов;
 - деактивировать окружение `open_rsai_detectors`.

## 1. Детектирование объектов местности 

### 1.1. Детектирование объектов на одиночном снимке

Детекторы объектов местности библиотеки OpenRSAI основаны на архитектуре Mask-RCNN и имеют предустановленные конфигурации (профили) для разных типов объектов, содержащиеся в модуле `mrcnn.open_rsai_config`. Профили содержат значения параметров сети, подобранные в результате проведенных экспериментальных исследований. Размер "окна" сети составляет 1024x1024 точки. Пример кода ниже демонстрирует детектирование растительности на заданном изображении 'sample.png'.

Внимание. Для запуска примера необходимо создать и разместить в каталоге тестовое изображение 'sample.png' или заменить имя 'sample.png' на путь к требуемому файлу в коде примера.  

```
import os
import cv2
import numpy as np

import mrcnn.model
import mrcnn.visualize

# Загрузка стандартного профился детектирования растительности
from mrcnn.open_rsai_config import GreeneryDetectConfig

# Файл весовых коэффициентов
weights_path = "./weights/mask_rcnn_green_0073.h5"

# Создание модели в режиме детектирования, загрузка весов
model = mrcnn.model.MaskRCNN(
    mode="inference",
    config=GreeneryDetectConfig(),
    model_dir=os.getcwd()
)

model.load_weights(
    filepath=weights_path,
    by_name=True
)

image = cv2.imread('sample.png')

# Непосредственное детектирование
results = model.detect([image], verbose=0)
r = results[0]

# Отрисовка масок детектированных объектов
color = (0, 1, 0)  
for i in range(r['rois'].shape[0]):
    mask = r['masks'][:, :, i]
    mrcnn.visualize.apply_mask ( image, mask, color )

image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
cv2.imwrite('output.png', image)
```

В приведенном примере необходимо заменить
- `weights_path` - путь к файлу весов ранее обученного детектора, в примере задан "./weights/mask_rcnn_green_0073.h5" из состава библиотеки;
- 'sample.png' - путь к изображению для детектирования;
- 'output.png' - путь к результирующему изображению с результатами детектирования.

### 1.2. Детектирование объектов на спутниковом покрытии

Библиотека OpenRSAI содержит функции для работы со спутниковыми покрытиями размерами более 10000 точек в каждом измерении. Для обработки покрытия библиотека содержит функцию `mrcnn.open_rsai_detect.coverage`, которая:
- "дробит" его на фрагменты заданного размера с перекрытием для удаления "слепых зон" на стыке фрагментов;
- выполняет детектирование в пределах фрагментов;
- использует многомасштабный подход - выполняет детектирование на фрагментах нескольких размеров (пирамиде), приведенных к размеру окна нейронной сети;
- объединяет результаты детектирования с разных фрагментов помощью растровой карты;
- сохраняет результат детектирования в ESRI Shapefile с географической координатной привязкой.

Функция `mrcnn.open_rsai_detect.coverage` принимает следующие параметры:
- `output_path` - путь для сохранения результирующей карты;
- `weights_file` - путь к файлу весовых коэффициентов;
- `work_region_shp` - путь файлу рабочей области - полигональный Shapefile, в котором первый полигон представляет границы рабочей области или ключ 'all' обработки для всего растрового покрытия;
- `path_to_raster` - путь к растровому покрытию;
- `config` - конфигурация сети для детектирования объектов;
- `tile_sizes` - набор размеров фрагментов для разбиения покрытия.

Результат сохраняется в слой, название которого совпадает с полем `config.NAME`.

Пример детектирование растительности на спутниковом покрытии:

```
import mrcnn.open_rsai_detect
from mrcnn.open_rsai_config import GreeneryDetectConfig

output_path     = '../data/out/'
weights_file    = './weights/mask_rcnn_green_0073.h5'
work_region_shp = '../data/green/vector/roi/roi.shp'
path_to_raster  = '../data/raster/Kursk/Bing_19_3395.tif'
config          = GreeneryDetectConfig()
tile_sizes      = mrcnn.open_rsai_detect.green_tile_sizes

mrcnn.open_rsai_detect.coverage(output_path, weights_file, work_region_shp, path_to_raster, config, tile_sizes)
```

Функция `coverage` имеет обертки, например `hydro` или `greenery`, которые в качестве `tile_sizes` подставляют оптимальные наборы пирамид фрагментов для разрешения ~0.5 м/точку. 

### 1.3. Модифицированная структура пирамиды фрагментов и параметров сети

При обработке покрытий средней детализации (>1.0 м/точку) рекомендается НЕ пропускать первый ярус пирамиды фрагментов и начинать обработку с масштаба 1:1, например [1024, 2048, 4096]. Кроме того, возможно использование собственной конфигурации, например, при детектировании на нескольких GPU (`GPU_COUNT`) или необходимости модификации порогов детектирования `DETECTION_MIN_CONFIDENCE` или `DETECTION_NMS_THRESHOLD`:   

```
import mrcnn.open_rsai_detect
from mrcnn.config import Config
from mrcnn.open_rsai_config import GREEN_CLASS_NAMES

class MultiGPUConfig(Config):
    NAME = "green"
    IMAGES_PER_GPU = 1
    GPU_COUNT = 1
    NUM_CLASSES = len(GREEN_CLASS_NAMES)
    STEPS_PER_EPOCH = 600
    VALIDATION_STEPS = 20

    LEARNING_RATE = 0.001

    BACKBONE = "resnet101"

    IMAGE_MIN_DIM = 1024
    IMAGE_MAX_DIM = 1024

    RPN_NMS_THRESHOLD = 0.65
    RPN_ANCHOR_SCALES = (32, 64, 128, 256, 512)
    DETECTION_MIN_CONFIDENCE = 0.8
    DETECTION_NMS_THRESHOLD = 0.35


output_path     = '../data/out/'
weights_file    = './weights/mask_rcnn_green_0073.h5'
work_region_shp = '../data/green/vector/roi/roi.shp'
path_to_raster  = '../data/raster/Kursk/Bing_19_3395.tif'
config          = MultiGPUConfig()
tile_sizes      = [1024, 2048, 4096]

mrcnn.open_rsai_detect.coverage(output_path, weights_file, work_region_shp, path_to_raster, config, tile_sizes)
```

# 2. Обучение детектора

## 2.1. Использование предустановленной схемы обучения

Детекторы библиотеки OpenRSAI используют архитектуру Mask-RCNN, которая обучается на наборах данных в формате MS COCO. Для работы с такими наборами библиотека предоставляет класс `CocoDataset`, находящийся в модуле `mrcnn.open_rsai_dataset`. Обучение модели выполняется с помощью функции `mrcnn.open_rsai_train.fine_tuned`, которая реализует наилучшую схему обучения, состоящую из следующих этапов:
- 40 эпох обучения сегментирующей части сети (heads training);
- 80 эпох тонкой настройки (fine-tune) всех слоев, начиная с 4-го;
- 30 эпох тонкой настройки (fine-tune) всей сети.

Функция `mrcnn.open_rsai_train.fine_tuned` принимает следующие параметры:
- `model` - экземпляр обучаемой сети Mask-RCNN;
- `dataset_train` - объект класса `CocoDataset`, содержащий обучающую часть выборки;
- `dataset_val` - объект класса `CocoDataset`, содержащий валидационную часть выборки;
- `config` - конфигурация сети для обучения.

Следует обратить внимание, что конфигурация для обучения отлична от детектирования и имеет соответствующее название, например `GreeneryTrainConfig`.

Пример обучения модели детектирования растительности:

```
import os

import mrcnn.utils
import mrcnn.config
import mrcnn.model
import mrcnn.visualize
from mrcnn import model as modellib, utils

import logging
logging.getLogger().setLevel(logging.ERROR)

from mrcnn.open_rsai_config import GreeneryTrainConfig
from mrcnn.open_rsai_dataset import CocoDataset, DEFAULT_DATASET_YEAR
import mrcnn.open_rsai_train

dataset_dir     = '../data/green/markup/vegetation_all/'
model_path      = './weights/mask_rcnn_coco.h5'

# Создание конфигурации и модели
config = GreeneryTrainConfig()
model = modellib.MaskRCNN(
    mode="training", 
    config=config,
    model_dir=os.getcwd()
)

# Загрузка весов сети
print("Loading weights ", model_path)
model.load_weights(
    model_path, 
    by_name=True, 
    exclude=["mrcnn_class_logits", "mrcnn_bbox_fc",  "mrcnn_bbox", "mrcnn_mask"]
)

# Загрузка обучающей части набора
dataset_train = CocoDataset()
image_ids = dataset_train.load_coco(dataset_dir, "train", year=DEFAULT_DATASET_YEAR)
print('dataset_train:', len(image_ids))
dataset_train.prepare()

# Загрузка валидационной части набора
dataset_val = CocoDataset()
val_type = "val" if DEFAULT_DATASET_YEAR in '2017' else "minival"
image_ids = dataset_val.load_coco(dataset_dir, val_type, year=DEFAULT_DATASET_YEAR)
print('dataset_val:', len(image_ids))
dataset_val.prepare()

# Трехступенчатое обучения с fine-tune всех слоев
mrcnn.open_rsai_train.fine_tuned ( model, dataset_train, dataset_val, config )
```

Весовые коэффициенты при обучении будут сохранены в каталог "green`yyyyMMdd`T`HHmm`", где yyyy-MM-dd HH:mm - дата и время начала обучения.

## 2.2. Использование собственной схемы обучения

Библиотека OpenRSAI допускает создание собственной схемы обучения, совместимой с Mask-RCNN, а именно, конфигурации сети, порядка и параметров обучения. Конфигурацию сети следует модифициовать при:
- обучении на GPU с объемом видеопамяти, отличным от 24Гб (`IMAGES_PER_GPU`);
- обучении на нескольких GPU (`GPU_COUNT`);
- совместном обучении на нескольких классах (`NUM_CLASSES`);
- изменении количества итераций за эпоху обучения (`STEPS_PER_EPOCH`) и валидации (`VALIDATION_STEPS`);
- изменении скорости обучения (`LEARNING_RATE`);
- использовании другого типа backbone RPN сети (`BACKBONE`).

Пример обучения конфигурацией на основе карты GeForce 3060 12 Gb c более быстрой RPN сетью 'resnet50':

```
import os

import mrcnn.utils
import mrcnn.config
import mrcnn.model
import mrcnn.visualize
from mrcnn import model as modellib, utils

import logging
logging.getLogger().setLevel(logging.ERROR)

from mrcnn.open_rsai_config import GreeneryTrainConfig
from mrcnn.open_rsai_dataset import CocoDataset, DEFAULT_DATASET_YEAR
import mrcnn.open_rsai_train

class CustomTrainConfig(GreeneryTrainConfig):
    IMAGES_PER_GPU = 2
    GPU_COUNT = 1
    BACKBONE = "resnet50"

dataset_dir     = '../data/green/markup/vegetation_all/'
model_path      = './weights/mask_rcnn_coco.h5'

# Создание конфигурации и модели
config = CustomTrainConfig()
config.display()

model = modellib.MaskRCNN(
    mode="training", 
    config=config,
    model_dir=os.getcwd()
)

# Загрузка весов сети
print("Loading weights ", model_path)
model.load_weights(
    model_path, 
    by_name=True, 
    exclude=["mrcnn_class_logits", "mrcnn_bbox_fc",  "mrcnn_bbox", "mrcnn_mask"]
)

# Загрузка обучающей части набора
dataset_train = CocoDataset()
image_ids = dataset_train.load_coco(dataset_dir, "train", year=DEFAULT_DATASET_YEAR)
print('dataset_train:', len(image_ids))
dataset_train.prepare()

# Загрузка валидационной части набора
dataset_val = CocoDataset()
val_type = "val" if DEFAULT_DATASET_YEAR in '2017' else "minival"
image_ids = dataset_val.load_coco(dataset_dir, val_type, year=DEFAULT_DATASET_YEAR)
print('dataset_val:', len(image_ids))
dataset_val.prepare()

# Трехступенчатое обучения с fine-tune всех слоев
mrcnn.open_rsai_train.fine_tuned ( model, dataset_train, dataset_val, config )
```