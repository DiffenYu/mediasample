#!/bin/bash -ex
[ -d "goturn-files" ] || git clone https://github.com/spmallick/goturn-files.git --depth=1
pushd goturn-files
cat goturn.caffemodel.zip* > goturn.caffemodel.zip
unzip goturn.caffemodel.zip
popd
cp goturn-files/goturn.caffemodel goturn-files/goturn.prototxt .
