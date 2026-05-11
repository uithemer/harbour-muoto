TARGET = sailfishos-uithemer-helperd
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

QT += core gui dbus
QT -= qml quick widgets

# Pull in IconApplier, DensityEnabler, ThemePackOps + their deps.
# FontApplier intentionally absent: it lives in src/gui/ because it is
# unprivileged once the GUI runs as defaultuser.
include(../ops/ops.pri)

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/auth.h \
    $$PWD/helperservice.h

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/auth.cpp \
    $$PWD/helperservice.cpp

# Qt5DBus is exposed via QT += dbus above; no extra pkgconfig needed.
#
# polkit-qt-core-1 is the GUI-less core of polkit-qt (just Authority,
# Subject, ...) and is what's packaged for the SailfishOS SDK target.
# Its .pc file ships but does not export a usable Cflags: line on the
# SFOS-5.0.0.62 target, so PKGCONFIG += polkit-qt-core-1 silently
# produces no -I and the build fails with
#   fatal error: PolkitQt1/Authority: No such file or directory
# Hardcode the include + lib instead. Paths are resolved against the
# qmake target sysroot at build time:
#   /usr/include/polkit-qt-1/PolkitQt1/Authority   (verified to exist)
#   libpolkit-qt-core-1                            (matches BuildRequires)
INCLUDEPATH += /usr/include/polkit-qt-1
LIBS        += -lpolkit-qt-core-1

target.path = /usr/libexec
INSTALLS += target
