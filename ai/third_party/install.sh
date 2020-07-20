#!/bin/bash -ex

SCRIPT_DIR=$(dirname $(readlink -f "$0"))
pushd $SCRIPT_DIR
source /opt/rh/devtoolset-7/enable

function install_gflags()
{
    [ -d "gflags" ] || git clone https://github.com/gflags/gflags.git
    pushd gflags
    [ -d "build" ] || mkdir build
    pushd build
    cmake ..
    make -j`nproc`
    popd
    popd

}

function install_glog()
{
    [ -d "glog" ] || git clone https://github.com/google/glog.git
    pushd glog
    [ -d "build" ] || mkdir build
    pushd build
    cmake ..
    make -j`nproc`
    popd
    popd

}

function pre_install_opencv()
{
    sudo yum install gstreamer1-devel.x86_64 -y
    sudo yum -y install libv4l-devel

    # Or you can just run yum install gstreamer1* -y
    sudo yum install -y \
        gstreamer1-plugins-base \
        gstreamer1-plugins-base \
        gstreamer1-plugins-good \
        gstreamer1-plugins-bad \
        gstreamer1-plugins-ugly \
        gstreamer1-libav \
        gstreamer1-doc \
        gstreamer1-tools \
        gstreamer1-plugins-base-devel

    sudo yum -y install gtk2-devel
    sudo yum -y install gtk3-devel
    sudo yum install gtkglext-devel
    sudo yum -y install qt5-qtbase-devel
    sudo yum install -y libpng-devel
    sudo yum install -y jasper-devel
    sudo yum install -y openexr-devel
    sudo yum install -y libwebp-devel
    sudo yum -y install libjpeg-turbo-devel
    sudo yum install -y freeglut-devel mesa-libGL mesa-libGL-devel
    sudo yum -y install libtiff-devel
    sudo yum -y install libdc1394-devel
    #sudo yum -y install tbb-devel eigen3-devel
    sudo yum -y install boost boost-thread boost-devel
    sudo yum -y install libv4l-devel
}

function install_opencv()
{
    pre_install_opencv

    OPENCV_TAG=4.2.0-openvino
    OPENCV_CONTRIB_TAG=4.2.0
    INSTALL_DIR=$SCRIPT_DIR/opencv_install
    OPENCV_CONTRIB_DIR=$SCRIPT_DIR/opencv_contrib
    [ -d "opencv" ] || git clone https://github.com/opencv/opencv.git

    [ -d "opencv_contrib" ] || git clone https://github.com/opencv/opencv_contrib.git
    pushd opencv_contrib
    git checkout $OPENCV_CONTRIB_TAG
    popd

    pushd opencv
    git checkout $OPENCV_TAG
    [ -d "build" ] || mkdir build

    pushd build
    cmake -DCMAKE_BUILD_TYPE=RELEASE \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
        -DWITH_GSTREAMER=ON \
        -DWITH_FFMPEG=ON \
        -DENABLE_CXX11=ON \
        -DINSTALL_C_EXAMPLES=ON \
        -DINSTALL_PYTHON_EXAMPLES=ON \
        -DWITH_V4L=ON \
        -DWITH_OPENGL=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DOPENCV_ENABLE_NONFREE=ON \
        -DWITH_INF_ENGINE=ON \
        -DWITH_TBB=OFF \
        -DENABLE_FAST_MATH=1 \
        -DOPENCV_EXTRA_MODULES_PATH=$SCRIPT_DIR/opencv_contrib/modules \
        -DBUILD_EXAMPLES=ON ..

        #-DHAVE_opencv_python3=ON \
        #-DWITH_INF_ENGINE=ON \
        #-DPYTHON_EXECUTABLE=/home/media/ws/venv/openvino/bin/python \
        #-DOPENCV_PYTHON3_INSTALL_PATH=/home/media/ws/venv/openvino/lib/python3.6/site-packages \
        #-DWITH_OPENCL=OFF \

        make -j`nproc`
        make install
        popd
        [ -f "compile_commands.json" ] || ln -s ./build/compile_commands.json compile_commands.json

        popd
    }

install_gflags
install_glog
install_opencv

popd

