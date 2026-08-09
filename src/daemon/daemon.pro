TARGET = harbour-muoto-helperd
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

QT += core dbus
QT -= gui qml quick widgets

# DensityEnable only — icon apply/restore is session Launcher1.
INCLUDEPATH += $$PWD $$PWD/../ops

HEADERS += \
    $$PWD/helperservice.h \
    $$PWD/../ops/densityenabler.h \
    $$PWD/../ops/dconfuser.h \
    $$PWD/../ops/filelock.h \
    $$PWD/../ops/spawner.h

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/helperservice.cpp \
    $$PWD/../ops/densityenabler.cpp \
    $$PWD/../ops/dconfuser.cpp \
    $$PWD/../ops/filelock.cpp \
    $$PWD/../ops/spawner.cpp

# The bus policy (/etc/dbus-1/system.d/org.muoto.Muoto1.conf) is the
# only gate. Once a method call has cleared dbus-daemon it is
# trusted, no PolkitQt1::checkAuthorizationSync round-trip.

target.path = /usr/libexec
INSTALLS += target
