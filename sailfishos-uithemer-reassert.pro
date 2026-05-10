TEMPLATE = app
TARGET = sailfishos-uithemer-reassert

CONFIG += c++11 console
CONFIG -= app_bundle

QT += core gui
QT -= qml quick widgets

INCLUDEPATH += src

SOURCES += \
    src/sailfishos-uithemer-reassert.cpp \
    src/desktopfile.cpp \
    src/iconmanifest.cpp \
    src/imageutil.cpp \
    src/iconapplier.cpp \
    src/iconpreviewcache.cpp \
    src/filelock.cpp

HEADERS += \
    src/desktopfile.h \
    src/iconmanifest.h \
    src/imageutil.h \
    src/iconapplier.h \
    src/iconpreviewcache.h \
    src/filelock.h

target.path = /usr/bin
INSTALLS += target
