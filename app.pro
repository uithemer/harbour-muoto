TARGET = sailfishos-uithemer

scripts.files = scripts/* tps/*
scripts.path = $$PREFIX/share/$$TARGET

service.files = service/*
service.path = $$PREFIX/share/$$TARGET/service

images.files = images/*
images.path = $$PREFIX/share/$$TARGET/images

appicons.files = appicons/*
appicons.path = /usr/share/icons/hicolor/

INSTALLS += scripts service images appicons

CONFIG += sailfishapp c++11

SOURCES += src/sailfishos-uithemer.cpp \
    src/spawner.cpp \
    src/themepackmodel.cpp \
    src/fontweightmodel.cpp \
    src/themepack.cpp \
    src/desktopfile.cpp \
    src/iconmanifest.cpp \
    src/imageutil.cpp \
    src/iconapplier.cpp \
    src/iconpreviewcache.cpp \
    src/iconpreviewprovider.cpp \
    src/filelock.cpp \
    src/fontapplier.cpp \
    src/densityenabler.cpp

OTHER_FILES += \
    qml/sailfishos-uithemer.qml \
    qml/common/Settings.qml \
    qml/components/AboutLanguage.qml \
    qml/components/AboutTranslator.qml \
    qml/components/BackgroundRectangle.qml \
    qml/components/BusyState.qml \
    qml/components/FontPreview.qml \
    qml/components/LabelSpacer.qml \
    qml/components/LabelText.qml \
    qml/components/Notification.qml \
    qml/components/themepacklistview/ThemePackItem.qml \
    qml/cover/CoverPage.qml \
    qml/cover/CoverConfirm.qml \
    qml/cover/CoverActionList1.qml \
    qml/cover/CoverActionList2.qml \
    qml/cover/CoverLabel.qml \
    qml/cover/FontPreviewCover.qml \
    qml/pages/ConfirmPage.qml \
    qml/pages/DensityPage.qml \
    qml/pages/MainPage.qml \
    qml/pages/OptionsPage.qml \
    qml/pages/RestorePage.qml \
    qml/pages/RestoreDDPage.qml \
    qml/pages/WelcomePage.qml \
    qml/pages/AboutPage.qml \
    qml/pages/GuidePage.qml \
    qml/pages/RecoveryPage.qml \
    rpm/* \
    sailfishos-uithemer.desktop \

CONFIG += sailfishapp_i18n

TRANSLATIONS +=  translations/*.ts

HEADERS += \
    src/spawner.h \
    src/themepackmodel.h \
    src/fontweightmodel.h \
    src/themepack.h \
    src/desktopfile.h \
    src/iconmanifest.h \
    src/imageutil.h \
    src/iconapplier.h \
    src/iconpreviewcache.h \
    src/iconpreviewprovider.h \
    src/filelock.h \
    src/fontapplier.h \
    src/densityenabler.h
