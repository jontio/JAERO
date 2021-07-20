#-------------------------------------------------
#
# Project created by QtCreator 2015-08-01T20:41:23
# Jonti Olds
#
#-------------------------------------------------

#the three tricky things will be libvorbis,libogg, and libcorrect
#the settings I have here are for my setup but other most likely will install the libraries
#so you will need to point everything to the right place.
#if you are having trubbles focus on things like "LIBS += -L$$OGG_PATH/src/.libs"
#remember to compile libvorbis,libogg, and libcorrect before compiling this

DEFINES += JAERO_VERSION=\\\"v1.0.4.12-alpha\\\"

QT       += multimedia core network gui svg sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets  printsupport

TARGET = JAERO
TEMPLATE = app

INSTALL_PATH = /opt/jaero
JFFT_PATH = C:/work/git_repos/JFFT/
EXT_DEP_PATH=C:/work/svn/JAERO

QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$JFFT_PATH

DEFINES += _USE_MATH_DEFINES

#for unit tests
CI {
  DEFINES += GENERATE_TEST_OUTPUT_FILES
  SOURCES += \
    tests/testall.cpp \
    tests/fftwrapper_tests.cpp \
    tests/fftrwrapper_tests.cpp \
    tests/jfastfir_tests.cpp \
    tests/jfastfir_data_input.cpp \
    tests/jfastfir_data_expected_output.cpp

  HEADERS += \
    tests/jfastfir_data.h

  LIBS += -lCppUTest

  DEFINES+= MATLAB_PATH=\\\"$${PWD}/matlab/\\\"
  DEFINES+= TEST_OUTPUT_PATH=\\\"$${PWD}/test_output/\\\"

} else {
  SOURCES += \
        main.cpp
}

SOURCES += mainwindow.cpp \
    coarsefreqestimate.cpp \
    DSP.cpp \
    mskdemodulator.cpp \
    audiomskdemodulator.cpp \
    gui_classes/console.cpp \
    gui_classes/qscatterplot.cpp \
    gui_classes/qspectrumdisplay.cpp \
    gui_classes/qled.cpp \
    fftwrapper.cpp \
    fftrwrapper.cpp \
    gui_classes/textinputwidget.cpp \
    gui_classes/settingsdialog.cpp \
    aerol.cpp \
    gui_classes/planelog.cpp \
    databasetext.cpp \
    oqpskdemodulator.cpp \
    audiooqpskdemodulator.cpp \
    burstoqpskdemodulator.cpp \
    audioburstoqpskdemodulator.cpp \
    arincparse.cpp \
    tcpserver.cpp \
    sbs1.cpp \
    tcpclient.cpp \
    burstmskdemodulator.cpp \
    audioburstmskdemodulator.cpp \
    jconvolutionalcodec.cpp \
    audiooutdevice.cpp \
    compressedaudiodiskwriter.cpp \
    $$JFFT_PATH/jfft.cpp \
    util/stdio_utils.cpp \
    util/file_utils.cpp \
    util/RuntimeError.cpp\
    audioreceiver.cpp


HEADERS  += mainwindow.h \
    coarsefreqestimate.h \
    DSP.h \
    mskdemodulator.h \
    audiomskdemodulator.h \
    gui_classes/console.h \
    gui_classes/qscatterplot.h \
    gui_classes/qspectrumdisplay.h \
    gui_classes/qled.h \
    fftwrapper.h \
    fftrwrapper.h \
    gui_classes/textinputwidget.h \
    gui_classes/settingsdialog.h \
    aerol.h \
    gui_classes/planelog.h \
    databasetext.h \
    oqpskdemodulator.h \
    audiooqpskdemodulator.h \
    burstoqpskdemodulator.h \
    audioburstoqpskdemodulator.h \
    arincparse.h \
    tcpserver.h \
    sbs1.h \
    tcpclient.h \
    burstmskdemodulator.h \
    audioburstmskdemodulator.h \
    jconvolutionalcodec.h \
    audiooutdevice.h \
    compressedaudiodiskwriter.h \
    $$JFFT_PATH/jfft.h \
    util/stdio_utils.h \
    util/file_utils.h \
    util/RuntimeError.h\
    audioreceiver.h



FORMS    += mainwindow.ui \
    gui_classes/settingsdialog.ui \
    gui_classes/planelog.ui

RESOURCES += \
    jaero.qrc

DISTFILES += \
    LICENSE \
    ../README.md \
    ../images/screenshot-win-main.png \
    ../images/screenshot-win-planelog.png

win32 {
RC_FILE = jaero.rc

}

# remove possible other optimization flags
#QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3

# add the desired -O3 if not present
#QMAKE_CXXFLAGS_RELEASE *= -O3



win32 {
#on windows the libcorrect dlls are here
INCLUDEPATH +=$$EXT_DEP_PATH/libcorrect/include
contains(QT_ARCH, i386) {
    #message("32-bit")
    LIBS += -L$$EXT_DEP_PATH/libcorrect/lib/32
} else {
    #message("64-bit")
    LIBS += -L$$EXT_DEP_PATH/libcorrect/lib/64
}
LIBS += -llibcorrect
}

win32 {
# libacars support
INCLUDEPATH +=$$EXT_DEP_PATH/libacars-2.1.2
LIBS += -L$$EXT_DEP_PATH/libacars-2.1.2/build/libacars
#in windows use the dynamic lib rather than the static one even when stattically compiling

LIBS += -lacars-2.dll
} else {
LIBS += -lacars-2
}

win32: LIBS += -L$$EXT_DEP_PATH/libzmq/lib/ -llibzmq-v120-mt-4_0_4

INCLUDEPATH += $$EXT_DEP_PATH/libzmq/include
DEPENDPATH += $$EXT_DEP_PATH/libzmq/include

#for static building order seems to matter
LIBS += -lcorrect -lvorbis -lvorbisenc -logg


#for audio compressor
#for static building order seems to matter
INCLUDEPATH += $$EXT_DEP_PATH/libvorbis-1.3.6/include
LIBS += -L$$EXT_DEP_PATH/libvorbis-1.3.6/lib/.libs -lvorbis -lvorbisenc
LIBS += -L$$EXT_DEP_PATH/libogg-1.3.3/src/.libs -logg

#desktop
desktop.path = /usr/share/applications
desktop.files += JAERO.desktop
INSTALLS += desktop

#icon
icon.path = $$INSTALL_PATH
icon.files += jaero.ico
INSTALLS += icon

#install sounds
soundsDataFiles.path = $$INSTALL_PATH/sounds/
soundsDataFiles.files = sounds/*.*
INSTALLS += soundsDataFiles

#install library
target.path=$$INSTALL_PATH
INSTALLS += target

# disable stupid deprecated-copy warnings
# cluttering up issues
QMAKE_CXXFLAGS += '-Wno-deprecated-copy'

#QMAKE_CXXFLAGS += '-Werror=format-security'

win32: LIBS += -L$$PWD/../../../svn/build-qcustomplot-Desktop_Qt_MinGW_w64_64bit_MSYS2-Release/release/ -lqcustomplot

INCLUDEPATH += $$PWD/../../../svn/qcustomplot
DEPENDPATH += $$PWD/../../../svn/qcustomplot

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../../../svn/build-qcustomplot-Desktop_Qt_MinGW_w64_64bit_MSYS2-Release/release/qcustomplot.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../../../svn/build-qcustomplot-Desktop_Qt_MinGW_w64_64bit_MSYS2-Release/release/libqcustomplot.a

#define where we store everything so when using the command line we don't make the main directory messy.
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/debug
    OBJECTS_DIR = $$PWD/tmp/debug/stuff
    MOC_DIR = $$PWD/tmp/debug/stuff
    UI_DIR = $$PWD/tmp/debug/stuff
    RCC_DIR = $$PWD/tmp/debug/stuff
} else {
    DESTDIR = $$PWD/release
    OBJECTS_DIR = $$PWD/tmp/release/stuff
    MOC_DIR = $$PWD/tmp/release/stuff
    UI_DIR = $$PWD/tmp/release/stuff
    RCC_DIR = $$PWD/tmp/release/stuff
}
