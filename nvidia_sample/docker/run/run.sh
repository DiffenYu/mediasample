#!/bin/bash -ex
IMAGE_NAME=nvidia/cuda:12.2.2-devel-ubuntu22.04_nvcodec_0.1

docker run -v ${PWD}:${PWD} --runtime=nvidia --gpus all --env NVIDIA_DRIVER_CAPABILITIES=all -it --rm ${IMAGE_NAME}

# insider container
#run command below
#apt install libnvidia-encode-525
#export LD_LIBRARY_PATH=/Downloads/Video_Codec_SDK_12.1.14/Lib/linux/stubs/x86_64:$LD_LIBRARY_PATH
#export CMAKE_PREFIX_PATH=$LD_LIBRARY_PATH
#cd /Downloads/Video_Codec_SDK_12.1.14/Samples
#mkdir build
#cd build
#cmake -G "Unix Makefiles" -DCMAKE_LIBRARY_PATH=/usr/local/cuda-12.2/lib64/stubs  -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda-12.2 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug ..
#make -j$(nproc)
