# Base image
FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get --yes install gcc g++ make cmake wget libgdal-dev libopencv-dev libeigen3-dev libboost-dev python3 python3-pip git

# Install sam dependencies
RUN pip install setuptools opencv-python pycocotools matplotlib onnxruntime onnx --break-system-packages &&\ 
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu118 --break-system-packages &&\ 
pip install git+https://github.com/facebookresearch/segment-anything.git --break-system-packages 

# Set current working directory
WORKDIR /project/open_rsai_docker

# Copy code from your local context to the image working directory
COPY . .

RUN cmake . && cmake --build .
