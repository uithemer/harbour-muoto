TARGET = sailfishos-uithemer-icond
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

QT += core gui
QT -= qml quick widgets

# Same ops set as the privileged daemon (IconApplier, etc.); one-shot
# --restore / --reassert / --refresh-originals then exit. Despite the
# trailing -d in the binary name this is *not* a long-lived process;
# the actual long-lived service is /usr/libexec/sailfishos-uithemer-helperd.
include(../ops/ops.pri)

INCLUDEPATH += $$PWD

SOURCES += $$PWD/sailfishos-uithemer-icond.cpp

target.path = /usr/bin
INSTALLS += target
