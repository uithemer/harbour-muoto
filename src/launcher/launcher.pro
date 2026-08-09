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

# Honour LIBDIR from rpm/sfdk (e.g. /usr/lib64 on aarch64); else Qt's libs dir.
isEmpty(LIBDIR): LIBDIR = $$[QT_INSTALL_LIBS]
target.path = $$LIBDIR

INSTALLS += target
