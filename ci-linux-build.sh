#!/bin/bash

#linux build (for github "ubuntu-latest")

#will get dependancies, build and install jaero

#fail on first error
set -e

#we will need sudo later may as well do a sudo now
if [[ ! $(sudo echo 0) ]]; then exit; fi

#install dependancies and build tools
sudo apt-get install qt5-default cpputest build-essential qtmultimedia5-dev cmake libvorbis-dev libogg-dev libqt5multimedia5-plugins checkinstall libqcustomplot-dev libqt5svg5-dev -y

#tmp JAERO
#git clone https://github.com/jontio/JAERO
#cd JAERO
#git checkout 2021
#cd ..

SCRIPT=$(realpath $0)
SCRIPTPATH=$(dirname $SCRIPT)
cd $SCRIPTPATH

#libacars
git clone https://github.com/szpajder/libacars
cd libacars && git checkout v1.3.1
#needed for github actions
git fetch --prune --unshallow --tags || true
git status > /dev/null 2>&1
PACKAGE_VERSION=$(git describe --tags --match 'v*' --dirty 2> /dev/null | tr -d v)
echo "PACKAGE_VERSION="$PACKAGE_VERSION
mkdir build
cd build
cmake ..
make
sudo checkinstall \
            --pkgsource="https://github.com/szpajder/libacars" \
            --pkglicense="MIT" \
            --maintainer="https://github.com/szpajder" \
            --pkgversion="$PACKAGE_VERSION" \
            --pkgrelease="1" \
            --pkgname=libacars-dev \
            --provides=libacars-dev \
            --summary="A library for decoding various ACARS message payloads" \
            --requires="" \
            -y
sudo ldconfig
cd ../..

#libcorrect
git clone https://github.com/quiet/libcorrect
cd libcorrect
#needed for github actions
git fetch --prune --unshallow --tags || true
git status > /dev/null 2>&1
PACKAGE_VERSION=1_$(git rev-parse HEAD | cut -c 1-8)
echo "PACKAGE_VERSION="$PACKAGE_VERSION
mkdir build
cd build
cmake ..
make
sudo checkinstall \
            --pkgsource="https://github.com/quiet/libcorrect" \
            --pkglicense="BSD-3-Clause License" \
            --maintainer="https://github.com/quiet" \
            --pkgversion="$PACKAGE_VERSION" \
            --pkgrelease="1" \
            --pkgname=libcorrect-dev \
            --provides=libcorrect-dev \
            --summary="C library for Convolutional codes and Reed-Solomon" \
            --requires="" \
            -y
sudo ldconfig
cd ../..

#JFFT
git clone https://github.com/jontio/JFFT

#libaeroambe
git clone https://github.com/jontio/libaeroambe
cd libaeroambe/mbelib-master
#needed for github actions
git fetch --prune --unshallow --tags || true
git status > /dev/null 2>&1
PACKAGE_VERSION=$(git describe --tags --match 'v*' --dirty 2> /dev/null | tr -d v)-g$(git rev-parse HEAD | cut -c 1-8)
PACKAGE_NAME=libaeroambe-dev
MAINTAINER=nobody
PACKAGE_SOURCE=https://github.com/jontio/libaeroambe
echo "PACKAGE_NAME="$PACKAGE_NAME
echo "PACKAGE_VERSION="$PACKAGE_VERSION
echo "MAINTAINER="$MAINTAINER
echo "PACKAGE_SOURCE="$PACKAGE_SOURCE
#build the old modified libmbe with mini-m patch
mkdir build
cd build
#need to turn of -fPIC for static linking
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
make
#dont install this as it's an old libmbe clone so instread delete the shared objects and just leave the static one to link with
rm $PWD/*.so*
cd ../../libaeroambe
#change install paths as i'm not sure of the QT define that gets the standard local user install path
sed -i 's/\$\$\[QT_INSTALL_HEADERS\]/\/usr\/local\/include/g' libaeroambe.pro
sed -i 's/\$\$\[QT_INSTALL_LIBS\]/\/usr\/local\/lib\//g' libaeroambe.pro
#add path for the static libmbe.a file
echo 'linux: LIBS += -L$$MBELIB_PATH/build/' >> libaeroambe.pro
echo "" >> libaeroambe.pro
#build the shared library and put it into a deb package
qmake
make
make INSTALL_ROOT=$PWD/release/${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1 install
cd release
cat <<EOT > control
Package: ${PACKAGE_NAME}
Source: ${PACKAGE_SOURCE}
Section: base
Priority: extra
Depends: qt5-default (>= 5.12)
Provides: ${PACKAGE_NAME}
Maintainer: ${MAINTAINER}
Version: ${PACKAGE_VERSION%_*}
License: MIT
Architecture: $(dpkg --print-architecture)
Description: A formal description of a mini-m decoder library
EOT
echo "" >> control
mkdir -p ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/DEBIAN
cp control ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/DEBIAN
dpkg-deb --build ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1
#install the deb package and go back to the main path
sudo apt install ./${PACKAGE_NAME}*.deb -y
sudo ldconfig
cd ../../..

#JAERO
cd $SCRIPTPATH
#needed for github actions
git fetch --prune --unshallow --tags || true
git status > /dev/null 2>&1
PACKAGE_VERSION=$(git describe --tags --match 'v*' --dirty 2> /dev/null | tr -d v)-g$(git rev-parse HEAD | cut -c 1-8)
PACKAGE_NAME=jaero
MAINTAINER=https://github.com/jontio
PACKAGE_SOURCE=https://github.com/jontio/JAERO
echo "PACKAGE_NAME="$PACKAGE_NAME
echo "PACKAGE_VERSION="$PACKAGE_VERSION
echo "MAINTAINER="$MAINTAINER
echo "PACKAGE_SOURCE="$PACKAGE_SOURCE
cd JAERO
#run unit tests
qmake CONFIG+="CI"
make
./JAERO -v
rm JAERO
qmake CONFIG-="CI"
make
make INSTALL_ROOT=$PWD/${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1 install
JAERO_INSTALL_PATH=$(cat JAERO.pro | sed -n -e 's/^INSTALL_PATH[|( ).]= //p')
JAERO_INSTALL_PATH=${JAERO_INSTALL_PATH//$'\r'/}
echo 'JAERO_INSTALL_PATH='${JAERO_INSTALL_PATH}
#add control
cat <<EOT > control
Package: ${PACKAGE_NAME}
Source: ${PACKAGE_SOURCE}
Section: base
Priority: extra
Depends: qt5-default (>= 5.12), qtmultimedia5-dev, libvorbis-dev, libogg-dev, libqt5multimedia5-plugins, libqcustomplot-dev, libqt5svg5-dev
Provides: ${PACKAGE_NAME}
Maintainer: ${MAINTAINER}
Version: ${PACKAGE_VERSION%_*}
License: MIT
Architecture: $(dpkg --print-architecture)
Description: Demodulate and decode Aero signals. These signals contain SatCom ACARS messages as used by planes beyond VHF ACARS range
EOT
echo "" >> control
mkdir -p ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/DEBIAN
cp control ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/DEBIAN
#add path command
mkdir -p ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/usr/local/bin
cat <<EOT > ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/usr/local/bin/jaero
#!/bin/bash
/opt/jaero/JAERO
EOT
chmod +x ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/usr/local/bin/jaero
#build and install package
dpkg-deb --build ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1
sudo apt install ./${PACKAGE_NAME}*.deb -y
sudo ldconfig
cd ../..

echo "done"
