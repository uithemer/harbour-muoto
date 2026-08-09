TEMPLATE = lib
TARGET = default-dynamic-icons

QT += dbus gui svg

CONFIG += c++14
CONFIG += plugin
CONFIG += no_plugin_name_prefix
CONFIG += link_pkgconfig
PKGCONFIG += mlite5
PKGCONFIG += sailfishsilica

INCLUDEPATH += ../../launcher
LIBS += -L../../launcher -lmuoto-launcher

ASSETS_PATH = /usr/share/harbour-muoto/dynamic-icons/default-dynamic-icons
DEFINES += ASSETS_PATH=\\\"$${ASSETS_PATH}\\\"

SOURCES += \
    clock.cpp \
    calendar.cpp \
    devicelockstatus.cpp

HEADERS += \
    devicelockstatus.h

target.path = /usr/share/harbour-muoto/dynamic-icons

assets.path = $$ASSETS_PATH
assets.files = \
    ../assets/icon-launcher-calendar.svg \
    ../assets/icon-launcher-clock.svg

INSTALLS += target assets
