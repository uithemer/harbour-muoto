TEMPLATE = lib
TARGET = muoto-launcher

QT += dbus gui svg
QT -= qml quick widgets

CONFIG += c++14
CONFIG += hide_symbols
CONFIG += link_pkgconfig
PKGCONFIG += mlite5
PKGCONFIG += sailfishsilica
PKGCONFIG += glib-2.0

DEFINES += MUOTO_LAUNCHER_BUILD_LIBRARY

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/../ops

include($$PWD/launcher.pri)

target.path = /usr/lib

INSTALLS += target
