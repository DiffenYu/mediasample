#!/bin/bash

usage() {
    echo "Usage:"
    echo "  ./build.sh ffmpeg"
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

install_x264() {
    echo "Downloading x264..."
    wget -c ftp://ftp.videolan.org/pub/videolan/x264/snapshots/x264-snapshot-20170531-2245-stable.tar.bz2
    tar -xvf x264-snapshot-20170531-2245-stable.tar.bz2

    echo "Building x264... "
    pushd x264-snapshot-20170531-2245-stable
    ./configure --prefix=$SRC_DIR --enable-shared
    make -s V=0
    make install
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
    local TAG="n4.4.3"
    pushd ${SRC_DIR}
    [[ ! -s "FFmpeg" ]] && git clone https://github.com/FFmpeg/FFmpeg.git
    pushd FFmpeg
    git checkout ${TAG}
    ./configure \
        --prefix=${INSTALL_DIR} \
        --enable-shared \
        --enable-asm

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
            build_ffmpeg
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

