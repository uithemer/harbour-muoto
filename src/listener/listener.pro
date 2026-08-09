TARGET = harbour-muoto-install-listener
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

QT += core dbus

INCLUDEPATH += $$PWD $$PWD/../ops

SOURCES += \
    main.cpp \
    installlistener.cpp \
    pktxwatch.cpp \
    ../ops/osupdateguard.cpp

HEADERS += \
    installlistener.h \
    pktxwatch.h \
    ../ops/osupdateguard.h

target.path = /usr/libexec
INSTALLS += target
