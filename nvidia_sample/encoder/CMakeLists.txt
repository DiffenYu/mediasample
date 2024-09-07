cmake_minimum_required(VERSION 3.10)
project(NvEncoderSample)

set(CMAKE_CXX_STANDARD 11)

# Define Debug-specific settings
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building in Debug mode")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEBUG -g")
endif()

# Define Release-specific settings (optional)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Building in Release mode")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
endif()

find_package(CUDA REQUIRED)
message(STATUS "Found CUDA: ${CUDA_VERSION}")
message(STATUS "CUDA_INCLUDE_DIRS: ${CUDA_INCLUDE_DIRS}")
message(STATUS "CUDA_LIBRARIES: ${CUDA_LIBRARIES}")
message(STATUS "CUDA_TOOLKIT_ROOT_DIR: ${CUDA_TOOLKIT_ROOT_DIR}")

find_library(CUVID_LIB nvcuvid HINTS /lib/x86_64-linux-gnu)
find_library(NVENCODEAPI_LIB nvidia-encode)

# Find nvcuvid library
find_library(NVCUVID_LIBRARY nvcuvid HINTS
    ${CUDA_TOOLKIT_ROOT_DIR}/lib64
    /usr/local/cuda/lib64
    /lib/x86_64-linux-gnu
)

# Find nvidia-encode library
find_library(NVENC_LIBRARY nvidia-encode HINTS
    ${CUDA_TOOLKIT_ROOT_DIR}/lib64
    /usr/local/cuda/lib64
    /lib/x86_64-linux-gnu
)

# include_directories(${CUDA_INCLUDE_DIRS} /path/to/Video_Codec_SDK/Interface)
include_directories(${CUDA_INCLUDE_DIRS} ../third_party/Video_Codec_SDK_12.0.16/Interface)
add_executable(NvEncoderSample main.cpp)
# target_link_libraries(NvEncoderSample ${CUDA_LIBRARIES} /path/to/Video_Codec_SDK/Lib/linux/stubs/x86_64/libnvidia-encode.so cuda)
target_link_libraries(NvEncoderSample ${CUDA_LIBRARIES} ${NVCUVID_LIBRARY} ${NVENC_LIBRARY} cuda)
