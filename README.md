# Open Remote Sensing AI Library Core (OpenRSAI-Core)

## Description

The OpenRSAI-Core library contains custom Eigen-powered geodata handlers, building's models on remote sensing imagery, algorithms for their detection and reconstruction (roof+projection+shade extraction) and training datasets preparing basics.

## Requirements

This project requires a number of dependencies to be installed on your system. All instructions below are tailored for Ubuntu 22.04 users. If you are using a different operating system, please adjust the commands accordingly.

## Structure

The Open Remote Sensing AI Library Core is provided with the following extension repositories:

1. The [OpenRSAI-Algos](https://github.com/miron77s/open_rsai_algos) adding trainable greenery and hydro detectors (this repo may be used as standalone).
2. The [OpenRSAI-Markup](https://github.com/miron77s/open_rsai_markup) appending dataset preparing toolchains to train detectors.
3. The [OpenRSAI-QGIS](https://github.com/miron77s/open_rsai_qgis_plugins) provides plugins to integrate OpenRSAI detectors into QGIS environment.
4. The [OpenRSAI-Data](https://github.com/miron77s/open_rsai_data) contains vector and downloadable raster data for detectors training and demonstration purposes alone with QGIS project files for visualization.

### Hardware

OpenRSAI-Core utilities can be run in both classic and AI modes:

 - Classic mode uses classic machine learning techniques and does not require GPU support. The only requirement is at least 16 Gb RAM.
 - AI mode for inference also require at least NVIDIA GeForce 3060 (minimum 8Gb GPU memory).
 - AI mode for buildings' detector training requires NVIDIA GeForce 4090 (24Gb GPU memory) for Yolo configuration, provided in this repository. For training on other GPUs modify Yolo configuration file according the following [guide](https://github.com/AlexeyAB/darknet?tab=readme-ov-file#how-to-train-to-detect-your-custom-objects).
 - Both modes require 6Gb of free disk space to download and extract required data on build and install stage.

### System Packages

Before installing the Python-specific dependencies, you need to install some system packages using `apt-get`. Open a terminal and run the following commands:

```
sudo apt-get update
sudo apt-get --yes install gcc g++ make cmake wget libgdal-dev libopencv-dev libeigen3-dev libboost-dev
sudo apt-get --yes install python3 python3-pip git mesa-common-dev
```

### Segment-Anything

After installing the system packages, you can install the required Python packages using `pip`. It is recommended to use a virtual environment to avoid conflicts with system-wide packages.
```
pip install setuptools opencv-python pycocotools matplotlib onnxruntime onnx --break-system-packages
```

For PyTorch with CUDA 11.8 support, install the packages from the specified PyTorch wheel index:
```
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu118 --break-system-packages
```

Finally, install the `Segment-Anything` library directly from the repository:
```
pip install git+https://github.com/facebookresearch/segment-anything.git --break-system-packages
```

For more information about `Segment-Anything` setup, see the [Installation section](https://github.com/facebookresearch/segment-anything/blob/main/README.md#installation).

## Compile and Installation (using CMake)

The root CMakeLists.txt automatically list all repository contents and build each item with `add_subdirectory`. Do not forget to pass your actual installation path to `CMAKE_INSTALL_PREFIX`. The build directory `build` also can be changed accoring preferences.

Before compiling check [Darknet configuration section](https://github.com/AlexeyAB/darknet?tab=readme-ov-file#how-to-compile-on-linux-using-make) for GPU, CUDNN support and architecture.

```
git clone https://github.com/miron77s/open_rsai
cd open_rsai
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=../open_rsai_distr -DENABLE_INSTALLER=ON
cmake --build build --parallel ${nproc}
cmake --install build
```

`ENABLE_INSTALLER` flag is designated to Darknet library enabling it install to `CMAKE_INSTALL_PREFIX` path.

Make sure that CUDA library binaries are available in `LD_LIBRARY_PATH` overvise you will be facing linker error. If so add libraries path to `LD_LIBRARY_PATH`:
```
export LD_LIBRARY_PATH=/usr/local/cuda-11.8/compat/:$LD_LIBRARY_PATH
```

## Special Thanks

We wish to thank Innovations Assistance Fund (Фонд содействия инновациям, https://fasie.ru/)
for their support in our project within Code-AI program (https://fasie.ru/press/fund/kod-ai/).
