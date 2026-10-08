#!/usr/bin/env bash

rm -rf build
mkdir build
cd build
cmake ../src
make

cd ..
ln -sf build/compile_commands.json compile_commands.json
