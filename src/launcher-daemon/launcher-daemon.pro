TARGET = harbour-muoto-launcher-icond
TEMPLATE = app

CONFIG += c++14
CONFIG += link_pkgconfig
PKGCONFIG += mlite5

QT += gui dbus svg

INCLUDEPATH += ../launcher
# filelock.h lives here; the launcher lib exports it but does not re-home it.
INCLUDEPATH += ../ops

LIBS += -L../launcher -lmuoto-launcher

# FileLock is not exported from libmuoto-launcher (no export macro), so every
# consumer compiles it in, as src/ops/ops.pri does for the GUI.
SOURCES += main.cpp \
    ../ops/filelock.cpp

target.path = /usr/libexec

INSTALLS += target
