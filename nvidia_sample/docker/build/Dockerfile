FROM nvidia/cuda:12.2.2-devel-ubuntu22.04

RUN apt-get update && apt-get install -y \
        vim \
        cmake
RUN apt-get install -y pkg-config \
        libavcodec-dev \
        libavformat-dev \
        libavutil-dev \
        libvulkan-dev \
        vulkan-tools \
        freeglut3-dev \
        libglew-dev

RUN apt-get install -y ffmpeg

RUN echo 'export PATH=/usr/local/cuda-12.2/bin:$PATH' >> ~/.bashrc && \
    echo 'export LD_LIBRARY_PATH=/usr/local/cuda-12.2/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc && \
    echo 'export CUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda-12.2' >> ~/.bashrc

CMD ["/bin/bash"]


