#!/bin/bash

mkdir -p build
cd build
cmake ..
cd ..
cmake --build ./build --config Debug --target all --