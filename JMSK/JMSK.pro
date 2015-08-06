#-------------------------------------------------
#
# Project created by QtCreator 2015-08-01T20:41:23
# Jonti Olds.
#
#-------------------------------------------------

QT       += multimedia core gui svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets  printsupport

TARGET = JMSK
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    coarsefreqestimate.cpp \
    DSP.cpp \
    mskdemodulator.cpp \
    ../qcustomplot/qcustomplot.cpp \
    ../varicode/varicode.c \
    audiomskdemodulator.cpp \
    gui_classes/console.cpp \
    gui_classes/qscatterplot.cpp \
    gui_classes/qspectrumdisplay.cpp \
    gui_classes/qled.cpp \
    varicodepipedecoder.cpp \
    varicodewrapper.cpp

HEADERS  += mainwindow.h \
    coarsefreqestimate.h \
    DSP.h \
    mskdemodulator.h \
    ../qcustomplot/qcustomplot.h \
    ../varicode/varicode.h \
    ../varicode/varicode_table.h \
    audiomskdemodulator.h \
    gui_classes/console.h \
    gui_classes/qscatterplot.h \
    gui_classes/qspectrumdisplay.h \
    gui_classes/qled.h \
    varicodepipedecoder.h \
    varicodewrapper.h \
    ../kiss_fft130/kissfft.hh

FORMS    += mainwindow.ui

RESOURCES += \
    jmsk.qrc

DISTFILES += \
    LICENSE \
    ../kiss_fft130/TIPS \
    ../kiss_fft130/CHANGELOG \
    ../kiss_fft130/COPYING \
    ../kiss_fft130/README \
    ../qcustomplot/changelog.txt \
    ../qcustomplot/GPL.txt \
    jmsk.rc

win32 {
RC_FILE = jmsk.rc
}
