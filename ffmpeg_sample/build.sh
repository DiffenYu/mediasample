#!/bin/bash

usage() {
    echo "Usage:"
    echo "  ./build.sh ffmpeg [openh264] [x264] [cuda] [qsv] [debug]"
    echo "  ./build.sh x264|openh264|vvdec|vvenc"
    echo "  ./build.sh sample"
}

this=$(pwd)

SRC_DIR="$this/deps_src"
DST_DIR="$this/deps_dst"
INSTALL_DIR="${this}/install"
[[ ! -d $SRC_DIR ]] && mkdir $SRC_DIR
[[ ! -d $DST_DIR ]] && mkdir $DST_DIR
[[ ! -d ${INSTALL_DIR} ]] && mkdir $INSTALL_DIR

install_deps() {
    sudo -E yum install gcc gcc-c++ nasm yasm -y 
}

install_opus() {
    echo "Downloading opus-1.1..."
    wget -c http://downloads.xiph.org/releases/opus/opus-1.1.tar.gz
    tar -xvzf opus-1.1.tar.gz

    echo "Building opus-1.1..."
    pushd opus-1.1
    ./configure --prefix=$SRC_DIR
    make -s V=0
    make install
    popd
}

build_openh264() {
    local BRANCH="master"
    pushd ${SRC_DIR}
    [[ ! -s "openh264" ]] && git clone https://github.com/cisco/openh264.git
    pushd openh264
    git checkout ${BRANCH}
    make -j$(nproc)
    make install-shared PREFIX=${INSTALL_DIR}
    popd
}

build_x264() {
    local BRANCH="stable"
    pushd ${SRC_DIR}
    [[ ! -s "x264" ]] && git clone https://github.com/mirror/x264.git
    pushd x264
    git checkout ${BRANCH}
    ./configure \
        --prefix=${INSTALL_DIR} \
        --enable-shared

    make -j$(nproc)
    make install
    popd
    popd
}

build_vvenc() {
    pushd ${SRC_DIR}
    git clone https://github.com/fraunhoferhhi/vvenc
    pushd vvenc
    make install-release-shared install-prefix=${INSTALL_DIR}
    popd
    popd
}

build_vvdec() {
    pushd ${SRC_DIR}
    git clone https://github.com/fraunhoferhhi/vvdec
    pushd vvdec
    make install-release-shared install-prefix=${INSTALL_DIR}
    popd
    popd
}

install_fdkaac() {
    local VERSION="0.1.4"
    local SRC="fdk-aac-${VERSION}.tar.gz"
    local SRC_URL="http://sourceforge.net/projects/opencore-amr/files/fdk-aac/${SRC}/download"
    local SRC_MD5SUM="e274a7d7f6cd92c71ec5c78e4dc9f8b7"

    echo "Downloading fdk-aac-${VERSION}"
    [[ ! -s ${SRC} ]] && wget -c ${SRC_URL} -O ${SRC}
    if ! (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) ; then
        rm -f ${SRC} && wget -c ${SRC_URL} -O ${SRC}
        (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) || (echo "Downloaded file ${SRC} is corrupted." && return 1)
    fi
    rm -fr fdk-aac-${VERSION}
    tar xf ${SRC}

    echo "Building fdk-aac-${VERSION}"
    pushd fdk-aac-${VERSION}
    ./configure --prefix=$SRC_DIR --enable-shared --enable-static
    make -s V=0 
    make install
    popd

}

install_ffmpeg() {
    local VERSION="3.1.2"
    local DIR="ffmpeg-${VERSION}"
    local SRC="${DIR}.tar.bz2"
    local SRC_URL="http://ffmpeg.org/releases/${SRC}"
    local SRC_MD5SUM="8095acdc8d5428b2a9861cb82187ea73"

    echo "Downloading ffmpeg-${VERSION}"
    [[ ! -s ${SRC} ]] && wget -c ${SRC_URL}
    if ! (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) ; then
        rm -f ${SRC} && wget -c ${SRC_URL}
        (echo "${SRC_MD5SUM} ${SRC}" | md5sum --check) || (echo "Downloaded file ${SRC} is corrupted." && return 1)
    fi
    rm -fr ${DIR}
    tar xf ${SRC}

    echo "Building ffmpeg-${VERSION}"
    pushd ${DIR}
    PKG_CONFIG_PATH=$SRC_DIR/lib/pkgconfig CFLAGS=-fPIC ./configure --prefix=$SRC_DIR --enable-shared --disable-libvpx --disable-vaapi --enable-libopus --enable-libfdk-aac --enable-nonfree --enable-libx264 --enable-gpl && make -j4 -s V=0 && make install
    popd

}


build_ffmpeg() {
    local tag="n4.4.3"
    local config_params=""
    local enable_vvc=false
    config_params="${config_params} --prefix=${INSTALL_DIR}"
    config_params="${config_params} --enable-shared"
    config_params="${config_params} --enable-asm"
    config_params="${config_params} --enable-gpl"
    while [[ $# -gt 0 ]]; do
        case $1 in
            debug)
                config_params="${config_params} --disable-optimizations"
                config_params="${config_params} --enable-debug=3"
                config_params="${config_params} --disable-small"
                config_params="${config_params} --disable-stripping"

                ;;
            x264)
                config_params="${config_params} --enable-libx264"
                ;;

            openh264)
                config_params="${config_params} --enable-libopenh264"

                ;;
            cuda)
                config_params="${config_params} --enable-ffnvcodec"
                config_params="${config_params} --enable-encoder=h264_nvenc"
                config_params="${config_params} --enable-hwaccel=h264_nvdec"
                config_params="${config_params} --enable-nvenc"
                config_params="${config_params} --enable-nvdec"
                config_params="${config_params} --enable-cuda"
                config_params="${config_params} --enable-cuvid"
                config_params="${config_params} --extra-cflags=-I/usr/local/cuda/include"
                config_params="${config_params} --extra-ldflags=-L/usr/local/cuda/lib64"

                ;;
            qsv)
                config_params="${config_params} --enable-libmfx"
                config_params="${config_params} --enable-encoder=h264_qsv"
                config_params="${config_params} --enable-decoder=h264_qsv"
                config_params="${config_params} --enable-encoder=hevc_qsv"
                config_params="${config_params} --enable-decoder=hevc_qsv"

                ;;
            vvcenc)
                tag="release/6.0"
                config_params="${config_params} --enable-libvvenc"
                enable_vvc=true

                ;;
            vvcdec)
                tag="release/6.0"
                config_params="${config_params} --enable-libvvdec"
                enable_vvc=true

                ;;
            * )
                echo "parameter: $1 is not supported"
                ;;
        esac
        shift
    done
    echo "${config_params}"

    pushd ${SRC_DIR}
    if [[ ! -s "ffmpeg" ]]; then
        git clone https://github.com/FFmpeg/FFmpeg.git ffmpeg
        pushd ffmpeg
        git checkout ${tag}
        if [ "$enable_vvc" = true ]; then
            echo "enable vvc"
            wget -O Add-support-for-H266-VVC.patch https://patchwork.ffmpeg.org/series/8365/mbox/
            git apply ./Add-support-for-H266-VVC.patch --exclude=libavcodec/version.h
        fi
    else
        pushd ffmpeg
    fi

    PKG_CONFIG_PATH=${INSTALL_DIR}/lib/pkgconfig \
    ./configure ${config_params}

    make -j$(nproc)
    make install
    popd
    popd
}

copy_libs() {
    cp -p $SRC_DIR/lib/*.so.* $DST_DIR
    cp -p $SRC_DIR/lib/*.so $DST_DIR
}

ffmpeg() {
    install_deps

    pushd $SRC_DIR

    install_x264
    install_fdkaac
    install_opus
    install_ffmpeg
    popd

    copy_libs
}

sample() {
    echo "Building sample..."
}

shopt -s extglob
if [[ $# -gt 0 ]]; then
    case $1 in
        usage )
            exit 0
            ;;
        ffmpeg )
            shift
            build_ffmpeg $@
            exit 0
            ;;
        openh264 )
            shift
            build_openh264
            exit 0
            ;;
        x264 )
            build_x264
            exit 0
            ;;
        vvenc )
            build_vvenc
            exit 0
            ;;
        vvdec )
            build_vvdec
            exit 0
            ;;
        sample )
            sample
            exit 0
            ;;
        * )
            echo 'Error: Wrong input parameter!'
            usage
            exit 1
            ;;
    esac
else
    usage
fi

