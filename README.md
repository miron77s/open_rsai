# Open Remote Sensing AI Library Core (OpenRSAI-Core)

## Description

The OpenRSAI-Core library contains custom Eigen-powered geodata handlers, building's models on remote sensing imagery, algorithms for their detection and reconstruction (roof+projection+shade extraction) and training datasets preparing basics.

## Requirements

This project requires a number of dependencies to be installed on your system. All instructions below are tailored for Ubuntu 22.04 users. If you are using a different operating system, please adjust the commands accordingly.

The requirements setup guide for Ubuntu 22.04 is available [here](https://github.com/miron77s/open_rsai/blob/main/tutorial/REQUIREMENTS.md).

## Structure

The Open Remote Sensing AI Library Core is provided with the following extension repositories:

1. The [OpenRSAI-Algos](https://github.com/miron77s/open_rsai_algos) adding trainable greenery and hydro detectors (this repo may be used as standalone).
2. The [OpenRSAI-Markup](https://github.com/miron77s/open_rsai_markup) appending dataset preparing toolchains to train detectors.
3. The [OpenRSAI-QGIS](https://github.com/miron77s/open_rsai_qgis_plugins) provides plugins to integrate OpenRSAI detectors into QGIS environment.
4. The [OpenRSAI-Data](https://github.com/miron77s/open_rsai_data) contains vector and downloadable raster data for detectors training and demonstration purposes alone with QGIS project files for visualization.

### Hardware

OpenRSAI project utilities requires the following hardware:

 - at least 16 Gb RAM;
 - NVIDIA GeForce 4090 (24Gb GPU memory);
 - 50Gb of free disk space to download and extract required data on build and install stage.

### System Packages

- Before installing the Python-specific dependencies, you need to install some system packages using `apt-get`. Open a terminal and run the following commands:

```
sudo apt-get update
sudo apt-get --yes install gcc g++ make cmake wget libgdal-dev libopencv-dev libeigen3-dev libboost-dev
sudo apt-get --yes install python3 python3-pip git mesa-common-dev
```

- Make sure you have [Anaconda](https://www.anaconda.com/products/individual) or [Miniconda](https://docs.conda.io/en/latest/miniconda.html) installed on your system to manage your environments and packages.

Conda environment is required to provide the OpenRSAI-Core the specific versions of torch, numpy, opencv and etc and prevent conflicts with the system environment.

### Segment-Anything

First create a Conda environment by running the following command:
```
conda create -n open_rsai_core_sam python=3.11 numpy=1.26
```  

Important! The environment name is restricted to `open_rsai_core_sam` due it is used in OpenRSAI-Core code.

Activate the newly created environment:

```
conda activate open_rsai_core_sam
```

Install GDAL library to handle SAM-2-WKT geometry conversion:
```
conda install gdal  
```

Install the required Python packages using `pip`: 
```
pip install setuptools opencv-python pycocotools matplotlib onnxruntime onnx tqdm
```

For PyTorch with CUDA 11.8 support, install the packages from the specified PyTorch wheel index:
```
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu118
```

Finally, install the `Segment-Anything` library directly from the repository:
```
pip install git+https://github.com/facebookresearch/segment-anything.git
```

Deactivate the environment:

```
conda deactivate
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
