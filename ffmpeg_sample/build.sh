#!/bin/bash

usage() {
    echo "Usage:"
    echo "> build separate component, use below build command"
    echo "  ./build.sh x264|x265|openh264|vvdec|vvenc|fdkaac|opus"
    echo "> build ffmpeg with optional component, use below command,
    need to make sure those optional components already built via above command."
    echo "  ./build.sh ffmpeg openh264|x264|x265|vvcenc|vvcdec|fdkaac|opus|extend_flv|cuda|qsv|debug"
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


build_openh264() {
    local BRANCH="master"
    pushd ${SRC_DIR}
    [[ ! -s "openh264" ]] && git clone https://github.com/cisco/openh264.git
    pushd openh264
    git checkout ${BRANCH}
    make -j$(nproc)
    make install-shared PREFIX=${INSTALL_DIR}
    popd
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

build_x265() {
    local BRANCH="stable"
    pushd ${SRC_DIR}
    [[ ! -s "x265" ]] && git clone --branch ${BRANCH} https://github.com/videolan/x265.git
    pushd x265/source
    [[ ! -s "build" ]] && mkdir build
    pushd build
    cmake -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} ..

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

# need to install autoconf && automake in mac via below commands
# brew install autoconf
# brew install atuomake
build_fdkaac() {
    local tag="v2.0.2"
    pushd ${SRC_DIR}
    [[ ! -s "fdk-aac" ]] && git clone -b ${tag} https://github.com/mstorsjo/fdk-aac.git
    pushd fdk-aac
    sh autogen.sh
    ./configure \
        --prefix=${INSTALL_DIR} \
        --enable-shared \
        --enable-static

    make -j$(nproc)
    make install
    popd
    popd
}

build_opus() {
    local tag="v1.4"
    pushd ${SRC_DIR}
    [[ ! -s "opus" ]] && git clone -b ${tag} https://github.com/xiph/opus.git
    pushd opus
    sh autogen.sh
    ./configure --prefix=${INSTALL_DIR}
    make -j$(nproc)
    make install
    popd
    popd
}

prepare_extend_flv() {
    local branch=4.3
    pushd ${SRC_DIR}
    [[ ! -s "ffmpeg_rtmp_h265" ]] && git clone --branch ${branch} https://github.com/runner365/ffmpeg_rtmp_h265.git
    cp ffmpeg_rtmp_h265/flv.h ${SRC_DIR}/ffmpeg/libavformat/
    cp ffmpeg_rtmp_h265/flvdec.c ${SRC_DIR}/ffmpeg/libavformat/
    cp ffmpeg_rtmp_h265/flvenc.c ${SRC_DIR}/ffmpeg/libavformat/
    popd
}

preare_nvcodec_header() {
    local branch=n11.1.5.2
    pushd ${SRC_DIR}
    [[ ! -s "nv-codec-headers" ]] && git clone --branch ${branch} https://github.com/FFmpeg/nv-codec-headers.git
    pushd nv-codec-headers
    make PREFIX=${INSTALL_DIR} install
    popd
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

            x265)
                config_params="${config_params} --enable-libx265"
                ;;

            openh264)
                config_params="${config_params} --enable-libopenh264"

                ;;
            cuda)
                preare_nvcodec_header
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
            fdkaac)
                config_params="${config_params} --enable-libfdk-aac"
                config_params="${config_params} --enable-nonfree"

                ;;
            opus)
                config_params="${config_params} --enable-libopus"

                ;;

            extend_flv)
                enable_extend_flv=true

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
        if [ "${enable_extend_flv}" = true ]; then
            echo "enable extend flv[hevc/vp8/vp9/opus]"
            prepare_extend_flv
        fi
    else
        pushd ffmpeg
        if [ "${enable_extend_flv}" = true ]; then
            if [ `grep -c "HEVC" libavformat/flv.h` -ne '0' ]; then
                echo "already prepred extend flv releated files"
            else
                echo "need to prepare"
                prepare_extend_flv
            fi
        fi
    fi

    PKG_CONFIG_PATH=${INSTALL_DIR}/lib/pkgconfig \
    ./configure ${config_params}

    make -j$(nproc)
    make install
    popd
    popd
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
        x265 )
            build_x265
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
        fdkaac)
            build_fdkaac
            exit 0
            ;;
        opus)
            build_opus
            exit 0
            ;;
        sample )
            sample
            exit 0
            ;;
        * )
            echo 'error: wrong input parameter!'
            usage
            exit 1
            ;;
    esac
else
    usage
fi

