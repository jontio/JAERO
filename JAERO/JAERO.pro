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

QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$LIBACARS_PATH/src

DEFINES += _USE_MATH_DEFINES

SOURCES += main.cpp\
        mainwindow.cpp \
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
    downloadmanager.cpp \
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
    ../../JFFT/jfft.cpp

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
    downloadmanager.h \
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
    ../../JFFT/jfft.h

# Tell the qcustomplot header that it will be used as library:
DEFINES += QCUSTOMPLOT_USE_LIBRARY
#qcustom plot is called different names on different systems
win32 {
#message("windows")
LIBS += -lqcustomplot2
} else {
#message("not windows")
LIBS += -lqcustomplot
}

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

LIBS += -lcorrect

# remove possible other optimization flags
#QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3

# add the desired -O3 if not present
#QMAKE_CXXFLAGS_RELEASE *= -O3

#for static building order seems to matter
LIBS += -lvorbis -lvorbisenc -logg -lacars

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
