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
    $$PWD/helperservice.h

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/helperservice.cpp

# The bus policy (/etc/dbus-1/system.d/org.uithemer.UiThemer1.conf) is the
# only gate. Once a method call has cleared dbus-daemon it is
# trusted, no PolkitQt1::checkAuthorizationSync round-trip. This
# removes the dependency on a polkit auth-agent being present in
# the user session (which lipstick does not always provide on
# community ports, and which appeared to silently deny our calls).

target.path = /usr/libexec
INSTALLS += target
