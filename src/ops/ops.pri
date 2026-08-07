# Shared "ops" sources. Included by GUI and minimal root helperd.
# Icon apply/restore lives in harbour-muoto-launcher-icond (session D-Bus).

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/iconapplier.h \
    $$PWD/iconpaths.h \
    $$PWD/iconpreviewcache.h \
    $$PWD/imageutil.h \
    $$PWD/filelock.h \
    $$PWD/spawner.h \
    $$PWD/dconfuser.h \
    $$PWD/densityenabler.h \
    $$PWD/osupdateguard.h \
    $$PWD/launcherdaemonctl.h

SOURCES += \
    $$PWD/iconapplier.cpp \
    $$PWD/iconpaths.cpp \
    $$PWD/iconpreviewcache.cpp \
    $$PWD/imageutil.cpp \
    $$PWD/filelock.cpp \
    $$PWD/spawner.cpp \
    $$PWD/dconfuser.cpp \
    $$PWD/densityenabler.cpp \
    $$PWD/osupdateguard.cpp \
    $$PWD/launcherdaemonctl.cpp
