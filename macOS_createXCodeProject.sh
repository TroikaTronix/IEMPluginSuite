#!/bin/sh

mkdir -p build
cd build

/Applications/CMake.app/Contents/bin/cmake .. -G Xcode \
  -DIEM_BUILD_VST3=ON \
  -DIEM_BUILD_VST2=OFF \
  -DIEM_BUILD_LV2=OFF \
  -DIEM_BUILD_STANDALONE=OFF \
  -DIEM_BUILD_AAX=OFF \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.14 \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"