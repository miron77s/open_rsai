# Open Remote Sensing AI Library Core (OpenRSAI-Core)

## Requirements

This project requires a number of dependencies to be installed on your system. All instructions below are tailored for Ubuntu 22.04 users. If you are using a different operating system, please adjust the commands accordingly.

### System Packages

Before installing the Python-specific dependencies, you need to install some system packages using `apt-get`. Open a terminal and run the following commands:

```
sudo apt-get update
sudo apt-get --yes install gcc g++ make cmake wget libgdal-dev libopencv-dev libeigen3-dev libboost-dev
sudo apt-get --yes install python3 python3-pip git
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

```
git clone https://github.com/miron77s/open_rsai
git submodule update --init --recursive
cd open_rsai
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=../open_rsai_distr
cmake --build build
cmake --install build
```

## Special Thanks

We wish to thank Innovations Assistance Fund (Фонд содействия инновациям, https://fasie.ru/)
for their support in our project within Code-AI program (https://fasie.ru/press/fund/kod-ai/).
