TARGET = harbour-muoto-install-listener
TEMPLATE = app

CONFIG += c++11 console
CONFIG -= app_bundle

QT += core dbus

SOURCES += \
    main.cpp \
    installlistener.cpp \
    pktxwatch.cpp

HEADERS += \
    installlistener.h \
    pktxwatch.h

target.path = /usr/libexec
INSTALLS += target
