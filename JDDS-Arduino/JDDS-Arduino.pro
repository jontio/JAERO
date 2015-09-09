
#This qmake file holds where the files for the Arduino JDDS are.
#I used Arduino IDE 1.6.4 to compile this project for the Arduino Uno
#See https://www.arduino.cc/ for IDE and Arduino related issues.
#Jonti 2015

DISTFILES += \
    libraries/Slip/examples/Echo/Echo.ino \
    libraries/Slip/license.txt \
    libraries/Slip/readme.txt \
    libraries/AD9850/examples/Sweep/Sweep.ino \
    libraries/TimerOne/examples/FanSpeed/FanSpeed.pde \
    libraries/TimerOne/examples/Interrupt/Interrupt.pde \
    libraries/TimerOne/README.md \
    jddsmsk/license.txt \
    libraries/readme.txt \
    eagle/jdds-arduino.brd \
    eagle/jdds-arduino.sch \
    eagle/readme.md \
    eagle/images/brd-layout.png \
    eagle/images/sch.png

HEADERS += \
    libraries/AD9850/AD9850.h \
    libraries/Slip/Slip.h \
    libraries/TimerOne/TimerOne.h \
    libraries/TimerOne/config/known_16bit_timers.h

SOURCES += \
    libraries/AD9850/AD9850.cpp \
    libraries/Slip/Slip.cpp \
    libraries/TimerOne/TimerOne.cpp \
    jddsmsk/jddsmsk.ino
