#-------------------------------------------------
#
# Project created by QtCreator 2016-02-04T15:59:10
#
#      Add in Run section for copy om mount disk mnt_imx
#      cp
#      %{CurrentProject:NativePath}/../Run_project_release/%{CurrentProject:Name} /home/dnadev/mnt_imx
#      or %{CurrentProject:NativePath}/../../Run_project_release/srv_dt /home/dnadev/mnt_imx/devsrv
#-------------------------------------------------

QT       += core
QT       += network
QT       += serialport
#QT       += widgets
QT       -= gui

VERSION = 1.00.13
COMPATIBLE_VERSION = 7.30
DEFINES += MIN_VERSION=\\\"$$COMPATIBLE_VERSION\\\"

#version 1.00.00 begin
#version 1.00.01 change error processing
#1.00.05 new state machine processing + add Linguist
#1.00.07 change error processing model
#1.00.09 small error was removed
# press cover add, barcode read procedure in close cover procedure add.
#1.00.10 video online read function add
#1.00.11 ARM version with call signal for processing network function from thread
#1.00.12 ARM remove main QThread and direct call of network function
#1.00.13 ARM USB rebuild
#        in can_commsock change function CAN_CloseChan
#        while (!(CANCh[chan].state&CC_RXRPL)) {  was CANCh[i]!
#        CANCh[chan].state=0; // clear "got reply" flag, close channel
DEFINES += APP_VERSION=\\\"$$VERSION\\\"
MOC_DIR = ../moc
OBJECTS_DIR = ../obj
include (../../config.pro)

TARGET = srv_dt
TEMPLATE = app

CONFIG += console
CONFIG += app
CONFIG -= app_bundle
CONFIG += exception

LIBS += -lusb-1.0

SOURCES += main.cpp\
          dtBehav.cpp \
          logobject.cpp \
          can_commsock.c \
          commobject.cpp \
          progerror.cpp \
          gpio.cpp

HEADERS +=dtBehav.h \
          logobject.h \
          progerror.h \
          can_ids.h \
          code_errors.h \
          can_commsock.h \
          commobject.h \
          digres.h \
          gpio.h
          ../request_dev.h


DESTDIR = ../../$${CURRENT_BUILD}

TRANSLATIONS +=Server_Dev_ru.ts

# remove possible other optimization flags
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE -= -O3
QMAKE_CFLAGS_RELEASE -= -O
QMAKE_CFLAGS_RELEASE -= -O1
QMAKE_CFLAGS_RELEASE -= -O2
QMAKE_CFLAGS_RELEASE -= -O3
#QMAKE_CXXFLAGS +=O0


















