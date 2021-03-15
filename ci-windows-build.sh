#!/bin/bash

#windows build (for github "windows-latest")
#this is for 64bit mingw and msys2 install. all is done on the command line.
#for unit testing add "CONFIG+="CI"" for qmake when building JAERO.

#without github actions...
#git clone https://github.com/jontio/JAERO
#cd JAERO
#git checkout 2021
#cd..

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

#libaeroambe
git clone https://github.com/jontio/libaeroambe
cd libaeroambe/mbelib-master
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX:PATH=/mingw64/ ..
mingw32-make
mingw32-make DESTDIR=/../ install
cp libmbe.dll /mingw64/bin/
cd ../../libaeroambe
qmake
mingw32-make
cp release/*.dll /mingw64/bin/
cp release/*.dll.a /mingw64/lib/
cd ../..

#JAERO
#github action already has already cloned JAERO
cd JAERO/JAERO
qmake
mingw32-make
mkdir release/jaero
cp release/JAERO.exe release/jaero/
cd release/jaero
windeployqt.exe --force JAERO.exe
cp /mingw64/bin/libstdc++-6.dll $PWD
cp /mingw64/bin/libgcc_s_seh-1.dll $PWD
cp /mingw64/bin/libvorbisenc-2.dll $PWD
cp /mingw64/bin/libvorbis-0.dll $PWD
cp /mingw64/bin/libogg-0.dll $PWD
cp /mingw64/bin/libacars.dll $PWD
cp /mingw64/lib/libcorrect.dll $PWD
cp /mingw64/bin/libwinpthread-1.dll $PWD
cp /mingw64/bin/zlib1.dll $PWD
cp /mingw64/bin/qcustomplot2.dll $PWD
cp /mingw64/bin/Qt5PrintSupport.dll $PWD
cp /mingw64/bin/libdouble-conversion.dll $PWD
cp /mingw64/bin/libicuin68.dll $PWD
cp /mingw64/bin/libicuuc68.dll $PWD
cp /mingw64/bin/libpcre2-16-0.dll $PWD
cp /mingw64/bin/libzstd.dll $PWD
cp /mingw64/bin/libharfbuzz-0.dll $PWD
cp /mingw64/bin/libpng16-16.dll $PWD
cp /mingw64/bin/libfreetype-6.dll $PWD
cp /mingw64/bin/libgraphite2.dll $PWD
cp /mingw64/bin/libglib-2.0-0.dll $PWD
cp /mingw64/bin/libicudt68.dll $PWD
cp /mingw64/bin/libbz2-1.dll $PWD
cp /mingw64/bin/libbrotlidec.dll $PWD
cp /mingw64/bin/libintl-8.dll $PWD
cp /mingw64/bin/libpcre-1.dll $PWD
cp /mingw64/bin/libbrotlicommon.dll $PWD
cp /mingw64/bin/libiconv-2.dll $PWD
cp /usr/lib/p7zip/7za.exe $PWD
cp /usr/bin/msys-stdc++-6.dll $PWD
cp /usr/bin/msys-gcc_s-seh-1.dll $PWD
cp /usr/bin/msys-2.0.dll $PWD
cp /mingw64/bin/aeroambe.dll $PWD
cp /mingw64/bin/libmbe.dll $PWD
cd ..
zip -r jaero.zip jaero