#!/bin/bash

#linux build (for raspberry pi raspberrian os)

#will get dependancies, build and install jaero
#NB will create folders at the same level as the JAERO folder. e.g if you cloned into ~/git/JAERO then you will have the following folders..
#~/git/JFFT
#~/git/JAERO
#~/git/libacars
#~/git/libcorrect
#~/git/libaeroambe

#fail on first error
set -e

#we will need sudo later may as well do a sudo now
if [[ ! $(sudo echo 0) ]]; then exit; fi

sudo apt update || true

#install dependancies and build tools
sudo apt-get install qt5-default cpputest build-essential qtmultimedia5-dev cmake libvorbis-dev libogg-dev libqt5multimedia5-plugins checkinstall libqcustomplot-dev libqt5svg5-dev libzmq3-dev gettext -y

#get script path
SCRIPT=$(realpath $0)
SCRIPTPATH=$(dirname $SCRIPT)
cd $SCRIPTPATH/..

#checkinstall on my buster raspberrian fails with make: *** [Makefile:332: cmake_check_build_system] Segmentation fault
#so clone and install a version that works on the pi
FOLDER="checkinstall"
URL="https://github.com/giuliomoro/checkinstall"
if [ ! -d "$FOLDER" ] ; then
    git clone $URL $FOLDER
    cd "$FOLDER"
else
    cd "$FOLDER"
    git pull $URL
fi
make
sudo make install
sudo ldconfig
cd ..


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
#needed for github actions
git fetch --prune --unshallow --tags || true
git status > /dev/null 2>&1
PACKAGE_VERSION=$(git describe --tags --match 'v*' --dirty 2> /dev/null | tr -d v)
PACKAGE_NAME=qmqtt-dev
MAINTAINER=https://github.com/emqx
PACKAGE_SOURCE=https://github.com/emqx/qmqtt
echo "PACKAGE_NAME="$PACKAGE_NAME
echo "PACKAGE_VERSION="$PACKAGE_VERSION
echo "MAINTAINER="$MAINTAINER
echo "PACKAGE_SOURCE="$PACKAGE_SOURCE
qmake
make
make INSTALL_ROOT=$PWD/release/${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1 install
cd release
cat <<EOT > control
Package: ${PACKAGE_NAME}
Source: ${PACKAGE_SOURCE}
Section: base
Priority: extra
Depends: qt5-default (>= 5.11)
Provides: ${PACKAGE_NAME}
Maintainer: ${MAINTAINER}
Version: ${PACKAGE_VERSION%_*}
License: Eclipse Public License 1.0
Architecture: $(dpkg --print-architecture)
Description: MQTT Client for Qt
EOT
echo "" >> control
mkdir -p ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/DEBIAN
cp control ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/DEBIAN
dpkg-deb --build ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1
#install the deb package and go back to the main path
sudo apt install ./${PACKAGE_NAME}*.deb -y
sudo ldconfig
cd ../..


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
#git checkout v1.3.1
#needed for github actions
git fetch --prune --unshallow --tags || true
git status > /dev/null 2>&1
PACKAGE_VERSION=$(git describe --tags --match 'v*' --dirty 2> /dev/null | tr -d v)
echo "PACKAGE_VERSION="$PACKAGE_VERSION
mkdir -p build
cd build
cmake ..
make
sudo checkinstall \
            -D \
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
sudo apt install --reinstall ./libacars-dev*.deb -y --allow-downgrades       
sudo ldconfig
#checkinstall with libacars seems to not always install the softlink
if ! compgen -G "/usr/local/lib/libacars-2.so" > /dev/null; then
    echo "stupid .so soft link missing grrr"
    find "/usr/local/lib/" -name "libacars-2.so.*"|while read fname; do
        echo "found something like it. creating softlink"
        echo "$fname"
        sudo ln -s $fname /usr/local/lib/libacars-2.so
        break;
    done
fi
cd ../..

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
#needed for github actions
git fetch --prune --unshallow --tags || true
git status > /dev/null 2>&1
PACKAGE_VERSION=1_$(git rev-parse HEAD | cut -c 1-8)
echo "PACKAGE_VERSION="$PACKAGE_VERSION
mkdir -p build
cd build
cmake ..
make
sudo checkinstall \
            -D \
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
sudo apt install --reinstall ./libcorrect-dev*.deb -y
sudo ldconfig
cd ../..

#JFFT
FOLDER="JFFT"
URL="https://github.com/jontio/JFFT"
if [ ! -d "$FOLDER" ] ; then
    git clone $URL $FOLDER
    cd "$FOLDER"
else
    cd "$FOLDER"
    git pull $URL
fi
cd ..

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
#needed for github actions
git fetch --prune --unshallow --tags || true
git status > /dev/null 2>&1
PACKAGE_VERSION=$(git describe --tags --match 'v*' --dirty 2> /dev/null | tr -d v)
PACKAGE_NAME=libaeroambe-dev
MAINTAINER=nobody
PACKAGE_SOURCE=https://github.com/jontio/libaeroambe
echo "PACKAGE_NAME="$PACKAGE_NAME
echo "PACKAGE_VERSION="$PACKAGE_VERSION
echo "MAINTAINER="$MAINTAINER
echo "PACKAGE_SOURCE="$PACKAGE_SOURCE
#build the old modified libmbe with mini-m patch
mkdir -p build
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
Depends: qt5-default (>= 5.11)
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
sudo apt install --reinstall ./${PACKAGE_NAME}*.deb -y  --allow-downgrades
sudo ldconfig
cd ../../..

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
#run unit tests
rm -fr JAERO
qmake CONFIG+="CI"
make
./JAERO -v
rm JAERO
#build for release
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
Depends: qt5-default (>= 5.11), qtmultimedia5-dev, libvorbis-dev, libogg-dev, libqt5multimedia5-plugins, libqcustomplot-dev, libqt5svg5-dev, libzmq3-dev
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
/opt/jaero/JAERO "\$@"
EOT
chmod +x ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/usr/local/bin/jaero
#basestation if available
if [ -f "../../basestation/basestation.sqb" ]; then
   echo "basestation.sqb found. including it in package"
   cp ../../basestation/basestation.sqb ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1/opt/jaero/
else
   echo "basestation.sqb not found. will be missing from package"
fi
#build and install package
dpkg-deb --build ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1
sudo apt install --reinstall ./${PACKAGE_NAME}*.deb -y
sudo ldconfig
cd ../..

#package
mkdir JAERO/bin
mkdir JAERO/bin/jaero
cp JAERO/JAERO/*.deb JAERO/bin/jaero
cp libacars/build/*.deb JAERO/bin/jaero
cp libcorrect/build/*.deb JAERO/bin/jaero
cp libaeroambe/libaeroambe/release/*.deb JAERO/bin/jaero
cp qmqtt/release/*.deb JAERO/bin/jaero
cd JAERO/bin
cat <<EOT > jaero/install.sh
#!/bin/bash
#installs built packages
sudo apt install --reinstall ./*.deb
sudo ldconfig
#checkinstall with libacars seems to not always install the softlink
if ! compgen -G "/usr/local/lib/libacars-2.so" > /dev/null; then
    echo "libacars .so soft link missing"
    find "/usr/local/lib/" -name "libacars-2.so.*"|while read fname; do
        echo "found something like it. creating softlink"
        echo "$fname"
        sudo ln -s $fname /usr/local/lib/libacars-2.so
        break;
    done
fi
EOT
chmod +x jaero/install.sh
cat <<EOT > jaero/uninstall.sh
#!/bin/bash
#removes built packages
sudo dpkg --remove qmqtt-dev libacars-dev libcorrect-dev libaeroambe-dev jaero
sudo rm /usr/local/lib/libacars-2.so
sudo ldconfig
EOT
chmod +x jaero/uninstall.sh
cat <<EOT > jaero/readme.md
# JAERO ${PACKAGE_VERSION}

### OS: $(lsb_release -d | cut -f 2)
### Build Date: $(date -u)

Cheers,<br>
ci-linux-build.sh
EOT
#compress
tar -czvf ${PACKAGE_NAME}_${PACKAGE_VERSION%_*}-1_linux_$(uname -m).tar.gz jaero
echo "done"
