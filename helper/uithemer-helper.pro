QT += core dbus
CONFIG += console
TEMPLATE = app
TARGET = uithemer-helper

SOURCES += src/main.cpp \
           src/uithemerhelper.cpp

HEADERS += src/uithemerhelper.h

# install path when packaged: /usr/sbin or /usr/libexec
