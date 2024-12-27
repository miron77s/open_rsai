# Сценарии работы с библиотекой

Перед первым запуском необходимо выполнить установку компонентов библиотеки согласно разделу инструкции ["Сборка и установка"](https://github.com/miron77s/open_rsai/blob/main/tutorial/TUTORIAL.md#%D1%81%D0%B1%D0%BE%D1%80%D0%BA%D0%B0-%D0%B8-%D1%83%D1%81%D1%82%D0%B0%D0%BD%D0%BE%D0%B2%D0%BA%D0%B0). 

## 1. Детектирование объектов местности 

### 1.1. Детектирование объектов на одиночном снимке

Детекторы объектов местности библиотеки OpenRSAI основаны на архитектуре Mask-RCNN и имеют предустановленные конфигурации (профили) для разных типов объектов, содержащиеся в модуле mrcnn.open_rsai_config. Профили содержат значения параметров сети, подобранные в результате проведенных экспериментальных исследований. Размер "окна" сети составляет 1024x1024 точки. Пример кода ниже демонстрирует детектирование растительности на заданном изображении 'sample.png'. 

```
import os
import cv2
import numpy as np

import mrcnn.model
import mrcnn.visualize

# Загрузка стандартного профился детектирования растительности
from mrcnn.open_rsai_config import GreeneryConfig

# Файл весовых коэффициентов
weights_path = "./weights/mask_rcnn_green_0073.h5"

# Создание модели в режиме детектирования, загрузка весов
model = mrcnn.model.MaskRCNN(
    mode="inference",
    config=GreeneryConfig(),
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
- `weights_path` - путь к файлу весов ранее обученного детектора
- 'sample.png' - путь к изображению для детектирования
- 'output.png' - путь к результирующему изображению с результатами детектирования

### 1.2. Детектирование объектов на спутниковом покрытии

Библиотека OpenRSAI содержит функции для работы со спутниковыми покрытиями размерами более 10000 точек в каждом измерении. Для обработки покрытия библиотека содержит функцию `coverage`, которая:
- "дробит" его на фрагменты заданного размера с перекрытием для удаления "слепых зон" на стыке фрагментов;
- выполняет детектирование в пределах фрагментов;
- использует многомасштабный подход - выполняет детектирование на фрагментах нескольких размеров (пирамиде), приведенных к размеру окна нейронной сети;
- объединяет результаты детектирования с разных фрагментов помощью растровой карты;
- сохраняет результат детектирования в ESRI Shapefile с географической координатной привязкой.

Функция `coverage` принимает следующие параметры:
- `output_path` - путь для сохранения результирующей карты;
- `weights_file` - путь к файлу весовых коэффициентов;
- `work_region_shp` - путь файлу рабочей области - полигональный Shapefile, в котором первый полигон представляет границы рабочей области или 'all' для всего покрытия;
- `path_to_raster` - путь к растровому покрытию;
- `config` - конфигурация сети для детектирования;
- `tile_sizes` - набор масштабов фрагментов для разбиения покрытия.

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

Функция `coverage` имеет обертки, например `hydro` или `greenery`, которые в качестве `tile_sizes` подставляют оптимальные наборы пирамид фрагментов для разрешения 1.0 м/точку. 

### 1.3. Модифицированная структура пирамиды фрагментов и параметров сети

При обработке покрытий высокой детализации (<0.5 м/точку) рекомендается пропускать первый ярус пирамиды фрагментов и начинать обработку с масштаба 1/2, например [2048, 4096]. Кроме того, возможно использование собственной конфигурации, например, при детектировании на нескольких GPU (`GPU_COUNT`) или необходимости модификации порогов детектирования `DETECTION_MIN_CONFIDENCE` или `DETECTION_NMS_THRESHOLD`:   

```
import mrcnn.open_rsai_detect
from mrcnn.open_rsai_config import GreeneryDetectConfig

class MultiGPUConfig(Config):
    NAME = "green"
    IMAGES_PER_GPU = 1
    GPU_COUNT = 4
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
tile_sizes      = [2048, 4096]

mrcnn.open_rsai_detect.coverage(output_path, weights_file, work_region_shp, path_to_raster, config, tile_sizes)
```