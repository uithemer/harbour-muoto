TARGET = harbour-muoto-launcher-icond
TEMPLATE = app

CONFIG += c++14
CONFIG += link_pkgconfig
PKGCONFIG += mlite5

QT += gui dbus svg

INCLUDEPATH += ../launcher

LIBS += -L../launcher -lmuoto-launcher

SOURCES += main.cpp

target.path = /usr/libexec

INSTALLS += target
