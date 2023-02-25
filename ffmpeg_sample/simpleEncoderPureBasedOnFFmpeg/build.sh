#!/bin/bash

export PKG_CONFIG_PATH=../../install/lib/pkgconfig:${PKG_CONFIG_PATH}
mkdir build
pushd build
cmake ..
make -j8
popd

# set the LD_LIBARAY_PATH to make sure use those libs you build when you run
# you can use ldd xxx to verify it
#export LD_LIBRARY_PATH=../../install/lib:$LD_LIBRARY_PATH


