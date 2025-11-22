#!/bin/sh

## script for creating the release binaries on
rm -rf build

rm -rf VST
rm -rf VST3
rm -rf LV2
rm -rf AAX
rm -rf Standalone

/Applications/CMake.app/Contents/bin/cmake -B build -DCMAKE_BUILD_TYPE=Debug -DIEM_BUILD_VST3=ON -DIEM_BUILD_VST2=OFF -DIEM_BUILD_LV2=OFF -DIEM_BUILD_STANDALONE=OFF -DIEM_BUILD_AAX=OFF -DCMAKE_OSX_DEPLOYMENT_TARGET=10.14 -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
/Applications/CMake.app/Contents/bin/cmake --build build --config Release -- -j 8

# cd ..
mkdir VST3
cp -r build/*/*_artefacts/Release/VST3/*.vst3 VST3/
