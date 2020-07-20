#!/bin/bash

[ -d "build" ] || mkdir build
pushd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
make -j`nproc`
popd
[ -f "compile_commands.json" ] || ln -s ./build/compile_commands.json compile_commands.json

