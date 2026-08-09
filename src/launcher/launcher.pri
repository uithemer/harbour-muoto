INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/muotolauncherglobal.h \
    $$PWD/launcherpaths.h \
    $$PWD/launchersettings.h \
    $$PWD/desktopentry.h \
    $$PWD/launchermanifest.h \
    $$PWD/iconprovider.h \
    $$PWD/svgiconrender.h \
    $$PWD/iconpack.h \
    $$PWD/iconpack_p.h \
    $$PWD/harbourthemepack.h \
    $$PWD/iconupdater.h \
    $$PWD/iconupdater_p.h \
    $$PWD/aliendalvikwatcher.h \
    $$PWD/dynamicicon.h \
    $$PWD/dynamicicon_p.h \
    $$PWD/iconresolve.h \
    $$PWD/overlayrender.h \
    $$PWD/overlayiconprovider.h \
    $$PWD/folderambient.h \
    $$PWD/launchericonops.h \
    $$PWD/launcherservice.h

SOURCES += \
    $$PWD/launcherpaths.cpp \
    $$PWD/launchersettings.cpp \
    $$PWD/desktopentry.cpp \
    $$PWD/launchermanifest.cpp \
    $$PWD/iconprovider.cpp \
    $$PWD/svgiconrender.cpp \
    $$PWD/iconpack.cpp \
    $$PWD/harbourthemepack.cpp \
    $$PWD/iconupdater.cpp \
    $$PWD/aliendalvikwatcher.cpp \
    $$PWD/dynamicicon.cpp \
    $$PWD/iconresolve.cpp \
    $$PWD/overlayrender.cpp \
    $$PWD/overlayiconprovider.cpp \
    $$PWD/folderambient.cpp \
    $$PWD/launchericonops.cpp \
    $$PWD/launcherservice.cpp \
    $$PWD/../ops/iconpaths.cpp \
    $$PWD/../ops/filelock.cpp \
    $$PWD/../ops/osupdateguard.cpp

PUBLICHEADERS += \
    $$PWD/muotolauncherglobal.h \
    $$PWD/dynamicicon.h \
    $$PWD/iconprovider.h \
    $$PWD/iconupdater.h \
    $$PWD/svgiconrender.h

publicheaderfiles.files = $$PUBLICHEADERS
publicheaderfiles.path = /usr/include/muoto-launcher

INSTALLS += publicheaderfiles
