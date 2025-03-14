# Инструкция по подготовке системы

Далее приведена инструкция по установке NVIDIA драйверов, CUDA 11.8 и CUDNN 8.7 в операционной системе Ubuntu 22.04.5. Проверена на системе с NVIDIA GeForce 4090.

## Благодарность

Инструкция создана на основе гайда [MihailCosmin](https://gist.github.com/MihailCosmin/affa6b1b71b43787e9228c25fe15aeba). Выражаем благодарность автору за его работу.

## Операционная система

Образ системы версии 22.04.5 взят с [официального сайта Ubuntu](https://releases.ubuntu.com/jammy/ubuntu-22.04.5-desktop-amd64.iso). Система установлена по умолчанию без скачивания сторонних библиотек (3rd party).

## Настройка

1. Обновление системы
```
sudo apt update && sudo apt upgrade -y
```

2. Установка необходимых пакетов и библиотек
```
sudo apt install g++ freeglut3-dev build-essential libx11-dev libxmu-dev libxi-dev libglu1-mesa libglu1-mesa-dev
```

3. Запрос и отображение перечня доступных драйверов
```
sudo add-apt-repository ppa:graphics-drivers/ppa
sudo apt update
ubuntu-drivers devices
```

4. Установка драйвера NVIDIA с зависимостями
```
sudo apt install libnvidia-common-570 libnvidia-gl-570 nvidia-driver-570 -y
```
Актуальная версия драйвера может отличаться. Зависит от релизов драйверов и используемого графического ускорителя.

5. Перезагрузка
```
sudo reboot now
```

6. Провека работоспособности драйвера
```
nvidia-smi
```
Должна быть отображена информация об ускорителе (или сообщение об ошибке, если что-то пошло не так). 

7. Подключение репозитория CUDA
```
sudo wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-ubuntu2204.pin
sudo mv cuda-ubuntu2204.pin /etc/apt/preferences.d/cuda-repository-pin-600
sudo apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/3bf863cc.pub
sudo add-apt-repository "deb https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/ /"
sudo apt update && sudo apt upgrade -y
```

8. Установка CUDA-11.8
```
sudo apt install cuda-11-8 -y
```
В процессе установка может произойти ошибка вследствие ошибок с зависимостями драйвера. Если это случилось, необходимо починить пакеты `sudo apt --fix-broken install`, после чего повторно запустить установку CUDA. 

9. Настройка путей для библиотек, заголовочных файлов и инструментов CUDA
```
echo 'export PATH=/usr/local/cuda-11.8/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda-11.8/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
sudo ldconfig
```

10. Проверка CUDA
```
nvcc -V
```
Если вместо информации о версии компилятора CUDA возвращается сообщение об ошибке, необходимо починить пакеты и выполнить установку, как описано в пунктах выше.

11. Установка cuDNN
```
CUDNN_TAR_FILE="cudnn-linux-x86_64-8.7.0.84_cuda11-archive.tar.xz"
sudo wget https://developer.download.nvidia.com/compute/redist/cudnn/v8.7.0/local_installers/11.8/cudnn-linux-x86_64-8.7.0.84_cuda11-archive.tar.xz
sudo tar -xvf ${CUDNN_TAR_FILE}
sudo mv cudnn-linux-x86_64-8.7.0.84_cuda11-archive cuda
```

12. Копирование CUDNN в каталог CUDA
```
sudo cp -P cuda/include/cudnn*.h /usr/local/cuda-11.8/include
sudo cp -P cuda/lib/libcudnn* /usr/local/cuda-11.8/lib64/
sudo chmod a+r /usr/local/cuda-11.8/lib64/libcudnn*
```



