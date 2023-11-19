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

#update system else we might have a version problem with the upcomeing install
pacman -Syu

pacman -S --needed --noconfirm git mingw-w64-x86_64-toolchain autoconf libtool mingw-w64-x86_64-pcre mingw-w64-x86_64-libxml2 mingw-w64-x86_64-cpputest mingw-w64-x86_64-qt5 mingw-w64-x86_64-cmake mingw-w64-x86_64-libvorbis zip p7zip unzip mingw-w64-x86_64-zeromq

#get script path
SCRIPT=$(realpath $0)
SCRIPTPATH=$(dirname $SCRIPT)
cd $SCRIPTPATH/..

#qmqtt
FOLDER="qmqtt"
URL="https://github.com/emqx/qmqtt.git"
if [ ! -d "$FOLDER" ] ; then
    git clone $URL $FOLDER
    cd "$FOLDER"
else
    cd "$FOLDER"
    git pull $URL
fi
qmake
mingw32-make
mingw32-make install
cd ..

#libacars
FOLDER="libacars"
URL="https://github.com/szpajder/libacars"
if [ ! -d "$FOLDER" ] ; then
    git clone $URL $FOLDER
    cd "$FOLDER"
else
    cd "$FOLDER"
    git pull $URL
fi
rm -fr build
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
FOLDER="libcorrect"
URL="https://github.com/quiet/libcorrect"
if [ ! -d "$FOLDER" ] ; then
    git clone $URL $FOLDER
    cd "$FOLDER"
else
    cd "$FOLDER"
    git pull $URL
fi
rm -fr build
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX:PATH=/mingw64/ ..
mingw32-make
mingw32-make DESTDIR=/../ install
cd ../..

#JFFT
FOLDER="JFFT"
URL="https://github.com/jontio/JFFT"
if [ ! -d "$FOLDER" ] ; then
    git clone $URL $FOLDER
else
    cd "$FOLDER"
    git pull $URL
	cd ..
fi

#libaeroambe
FOLDER="libaeroambe"
URL="https://github.com/jontio/libaeroambe"
if [ ! -d "$FOLDER" ] ; then
    git clone $URL $FOLDER
    cd "$FOLDER"
else
    cd "$FOLDER"
    git pull $URL
fi
cd mbelib-master
rm -fr build
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
echo "jaero make done"
mkdir release/jaero
cp release/JAERO.exe release/jaero/
cd release/jaero
echo "starting windeployqt"
windeployqt.exe --no-translations --force JAERO.exe
echo "deploy done"
echo "copying dlls"
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
cp /mingw64/bin/libicuin74.dll $PWD
cp /mingw64/bin/libicuuc74.dll $PWD
cp /mingw64/bin/libpcre2-16-0.dll $PWD
cp /mingw64/bin/libpcre2-8-0.dll $PWD
cp /mingw64/bin/libzstd.dll $PWD
cp /mingw64/bin/libharfbuzz-0.dll $PWD
cp /mingw64/bin/libpng16-16.dll $PWD
cp /mingw64/bin/libfreetype-6.dll $PWD
cp /mingw64/bin/libgraphite2.dll $PWD
cp /mingw64/bin/libglib-2.0-0.dll $PWD
cp /mingw64/bin/libicudt74.dll $PWD
cp /mingw64/bin/libbz2-1.dll $PWD
cp /mingw64/bin/libbrotlidec.dll $PWD
cp /mingw64/bin/libintl-8.dll $PWD
cp /mingw64/bin/libpcre-1.dll $PWD
cp /mingw64/bin/libbrotlicommon.dll $PWD
cp /mingw64/bin/libiconv-2.dll $PWD
cp /mingw64/bin/libzmq.dll $PWD
cp /mingw64/bin/libsodium-26.dll $PWD
cp /mingw64/bin/libmd4c.dll $PWD
cp /mingw64/bin/libjpeg-8.dll $PWD
#7za.exe not needed anymore
#cp /usr/lib/p7zip/7za.exe $PWD
#cp /usr/bin/msys-stdc++-6.dll $PWD
#cp /usr/bin/msys-gcc_s-seh-1.dll $PWD
#cp /usr/bin/msys-2.0.dll $PWD
cp /mingw64/bin/aeroambe.dll $PWD
cp /mingw64/bin/libmbe.dll $PWD
cp /mingw64/bin/libxml2-2.dll $PWD
cp /mingw64/bin/liblzma-5.dll $PWD
cp /mingw64/bin/libsqlite3-0.dll $PWD
cp /mingw64/bin/Qt5Qmqtt.dll $PWD
echo "copying dlls done"
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
