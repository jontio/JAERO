#-------------------------------------------------
#
# Project created by QtCreator 2015-08-01T20:41:23
# Jonti Olds
#
#-------------------------------------------------

QT       += multimedia core network gui svg sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets  printsupport

TARGET = JAERO
TEMPLATE = app

#message("QT_ARCH is \"$$QT_ARCH\"");
contains(QT_ARCH, i386) {
    #message("32-bit")
    DEFINES += kiss_fft_scalar=float
} else {
    #message("64-bit")
    DEFINES += kiss_fft_scalar=double
}

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
    varicodewrapper.cpp \
    ../kiss_fft130/kiss_fft.c \
    fftwrapper.cpp \
    fftrwrapper.cpp \
    ../kiss_fft130/kiss_fftr.c \
    mskmodulator.cpp \
    audiomskmodulator.cpp \
    varicodepipeencoder.cpp \
    gui_classes/textinputwidget.cpp \
    gui_classes/settingsdialog.cpp \
    aerol.cpp \
    ../viterbi-xukmin/viterbi.cpp \
    gui_classes/planelog.cpp \
    downloadmanager.cpp \
    databasetext.cpp

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
    ../kiss_fft130/_kiss_fft_guts.h \
    ../kiss_fft130/kiss_fft.h \
    fftwrapper.h \
    fftrwrapper.h \
    ../kiss_fft130/kiss_fftr.h \
    mskmodulator.h \
    audiomskmodulator.h \
    varicodepipeencoder.h \
    gui_classes/textinputwidget.h \
    gui_classes/settingsdialog.h \
    aerol.h \
    ../viterbi-xukmin/viterbi.h \
    gui_classes/planelog.h \
    downloadmanager.h \
    databasetext.h

FORMS    += mainwindow.ui \
    gui_classes/settingsdialog.ui \
    gui_classes/planelog.ui

RESOURCES += \
    jaero.qrc

DISTFILES += \
    LICENSE \
    ../kiss_fft130/TIPS \
    ../kiss_fft130/CHANGELOG \
    ../kiss_fft130/COPYING \
    ../kiss_fft130/README \
    ../qcustomplot/changelog.txt \
    ../qcustomplot/GPL.txt \
    ../README.md \
    ../viterbi-xukmin/LICENSE.md \
    ../viterbi-xukmin/README.md \
    ../images/screenshot-win-main.png \
    ../images/screenshot-win-planelog.png

win32 {
RC_FILE = jaero.rc
}
