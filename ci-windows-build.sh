#!/bin/bash

#windows build (for github "windows-latest")
#this is for 64bit mingw and msys2 install. all is done on the command line.

#will get dependancies, build and install jaero
#NB will create folders at the same level as the JAERO folder. e.g if you cloned into ~/git/JAERO then you will have the following folders..
#~/git/JFFT
#~/git/JAERO
#~/git/libacars
#~/git/libcorrect
#~/git/libaeroambe

#fail on first error
set -e

pacman -S --needed --noconfirm git mingw-w64-x86_64-toolchain autoconf libtool mingw-w64-x86_64-cpputest mingw-w64-x86_64-qt5 mingw-w64-x86_64-cmake mingw-w64-x86_64-libvorbis zip p7zip unzip mingw-w64-x86_64-zeromq

#get script path
SCRIPT=$(realpath $0)
SCRIPTPATH=$(dirname $SCRIPT)
cd $SCRIPTPATH/..

#libacars
git clone https://github.com/szpajder/libacars
#cd libacars && git checkout v1.3.1
cd libacars
mkdir build && cd build
sed -i -e 's/.*find_library(LIBM m REQUIRED).*/# find_library(LIBM m REQUIRED)/' ../libacars/CMakeLists.txt
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

#basestation
$SCRIPTPATH/ci-create-basestation.sh

#JAERO
cd $SCRIPTPATH
#needed for github actions
git fetch --prune --unshallow --tags || true
git status > /dev/null 2>&1
PACKAGE_VERSION=$(git describe --tags --match 'v*' --dirty 2> /dev/null | tr -d v)
PACKAGE_NAME=jaero
MAINTAINER=https://github.com/jontio
PACKAGE_SOURCE=https://github.com/jontio/JAERO
echo "PACKAGE_NAME="$PACKAGE_NAME
echo "PACKAGE_VERSION="$PACKAGE_VERSION
echo "MAINTAINER="$MAINTAINER
echo "PACKAGE_SOURCE="$PACKAGE_SOURCE
cd JAERO
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
cp /mingw64/bin/libacars-2.dll $PWD
cp /mingw64/lib/libcorrect.dll $PWD
cp /mingw64/bin/libwinpthread-1.dll $PWD
cp /mingw64/bin/zlib1.dll $PWD
cp /mingw64/bin/qcustomplot2.dll $PWD
cp /mingw64/bin/Qt5PrintSupport.dll $PWD
cp /mingw64/bin/libdouble-conversion.dll $PWD
cp /mingw64/bin/libicuin69.dll $PWD
cp /mingw64/bin/libicuuc69.dll $PWD
cp /mingw64/bin/libpcre2-16-0.dll $PWD
cp /mingw64/bin/libzstd.dll $PWD
cp /mingw64/bin/libharfbuzz-0.dll $PWD
cp /mingw64/bin/libpng16-16.dll $PWD
cp /mingw64/bin/libfreetype-6.dll $PWD
cp /mingw64/bin/libgraphite2.dll $PWD
cp /mingw64/bin/libglib-2.0-0.dll $PWD
cp /mingw64/bin/libicudt69.dll $PWD
cp /mingw64/bin/libbz2-1.dll $PWD
cp /mingw64/bin/libbrotlidec.dll $PWD
cp /mingw64/bin/libintl-8.dll $PWD
cp /mingw64/bin/libpcre-1.dll $PWD
cp /mingw64/bin/libbrotlicommon.dll $PWD
cp /mingw64/bin/libiconv-2.dll $PWD
cp /mingw64/bin/libzmq.dll $PWD
cp /mingw64/bin/libsodium-23.dll $PWD
cp /mingw64/bin/libmd4c.dll $PWD
#7za.exe not needed anymore
#cp /usr/lib/p7zip/7za.exe $PWD
#cp /usr/bin/msys-stdc++-6.dll $PWD
#cp /usr/bin/msys-gcc_s-seh-1.dll $PWD
#cp /usr/bin/msys-2.0.dll $PWD
cp /mingw64/bin/aeroambe.dll $PWD
cp /mingw64/bin/libmbe.dll $PWD
#basestation if available
if [ -f "../../../../basestation/basestation.sqb" ]; then
   echo "basestation.sqb found. including it in package"
   cp ../../../../basestation/basestation.sqb $PWD
else
   echo "basestation.sqb not found. will be missing from package"
fi
#add readme
cat <<EOT > readme.md
# JAERO ${PACKAGE_VERSION}

### OS Name: $(systeminfo | sed -n -e 's/^OS Name://p' | awk '{$1=$1;print}')
### OS Version: $(systeminfo | sed -n -e 's/^OS Version://p' | awk '{$1=$1;print}')
### System Type: $(systeminfo | sed -n -e 's/^System Type://p' | awk '{$1=$1;print}')
### Build Date: $(date -u)

Cheers,<br>
ci-windows-build.sh
EOT
#compress
cd ..
zip -r ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1_win_$(uname -m).zip jaero
