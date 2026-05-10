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

# Qt5DBus + polkit-qt-core-1 are runtime-required. polkit-qt-core-1 is
# the GUI-less core of polkit-qt (just Authority, Subject, ...); it is
# what is packaged for the SailfishOS SDK target. We never need the
# Agent / GUI sub-libraries because lipstick already runs the user-side
# auth agent.
CONFIG += link_pkgconfig
PKGCONFIG += polkit-qt-core-1 Qt5DBus

target.path = /usr/libexec
INSTALLS += target
