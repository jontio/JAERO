#!/bin/bash
PATH=/mingw64/bin:$PATH

#libacars
git clone https://github.com/szpajder/libacars
cd libacars && git checkout v1.3.1
mkdir build && cd build
sed -i -e 's/.*find_library(LIBM m REQUIRED).*/# find_library(LIBM m REQUIRED)/' ../src/libacars/CMakeLists.txt
cmake -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX:PATH=/mingw64/ ..
mingw32-make
mingw32-make DESTDIR=/../ install
cd ../..

#qcustomplot
wget -c https://www.qcustomplot.com/release/2.0.1/QCustomPlot.tar.gz -O - | tar -xz
cd qcustomplot
wget -c https://www.qcustomplot.com/release/2.0.1/QCustomPlot-sharedlib.tar.gz -O - | tar -xz
cd qcustomplot-sharedlib/sharedlib-compilation/
qmake
mingw32-make.exe
cp release/qcustomplot2.dll /mingw64/bin/
cp release/libqcustomplot2.dll.a /mingw64/lib/
cp debug/qcustomplotd2.dll /mingw64/bin/
cp debug/libqcustomplotd2.dll.a /mingw64/lib/
cp ../../qcustomplot.h /mingw64/include/
cd ../../..

#libcorrect
git clone https://github.com/quiet/libcorrect
cd libcorrect
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX:PATH=/mingw64/ ..
mingw32-make
mingw32-make DESTDIR=/../ install
cd ../..

#JFFT
git clone https://github.com/jontio/JFFT

#JAERO
git clone https://github.com/jontio/JAERO
cd JAERO
git checkout 2021
cd JAERO
qmake
mingw32-make
mkdir build
cp release/JAERO.exe build/
cd build
windeployqt.exe --force JAERO.exe
cp /mingw64/bin/libstdc++-6.dll $PWD
cp /mingw64/bin/libgcc_s_seh-1.dll $PWD
cp /mingw64/bin/libvorbisenc-2.dll $PWD
cp /mingw64/bin/libvorbis-0.dll $PWD
cp /mingw64/bin/libogg-0.dll $PWD
cp /mingw64/bin/libacars.dll $PWD
cp /mingw64/lib/libcorrect.dll $PWD