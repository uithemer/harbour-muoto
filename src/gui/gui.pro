TARGET = sailfishos-uithemer
TEMPLATE = app

CONFIG += sailfishapp c++11
CONFIG += sailfishapp_i18n

QT += dbus

# Shared ops (IconApplier, DensityEnabler, file lock, spawner, manifest,
# desktopfile, imageutil, iconpreviewcache, themepack-ops). FontApplier
# and FontWeightModel stay GUI-only because they are unprivileged and
# only the QML side ever touches them.
include(../ops/ops.pri)

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/themepack.h \
    $$PWD/themepackmodel.h \
    $$PWD/fontweightmodel.h \
    $$PWD/fontapplier.h \
    $$PWD/iconpreviewprovider.h \
    $$PWD/helperclient.h

SOURCES += \
    $$PWD/sailfishos-uithemer.cpp \
    $$PWD/themepack.cpp \
    $$PWD/themepackmodel.cpp \
    $$PWD/fontweightmodel.cpp \
    $$PWD/fontapplier.cpp \
    $$PWD/iconpreviewprovider.cpp \
    $$PWD/helperclient.cpp

ROOT = $$PWD/../..

OTHER_FILES += \
    $$ROOT/qml/sailfishos-uithemer.qml \
    $$ROOT/qml/common/Settings.qml \
    $$ROOT/qml/components/AboutLanguage.qml \
    $$ROOT/qml/components/AboutTranslator.qml \
    $$ROOT/qml/components/BackgroundRectangle.qml \
    $$ROOT/qml/components/BusyState.qml \
    $$ROOT/qml/components/FontPreview.qml \
    $$ROOT/qml/components/LabelSpacer.qml \
    $$ROOT/qml/components/LabelText.qml \
    $$ROOT/qml/components/Notification.qml \
    $$ROOT/qml/components/themepacklistview/ThemePackItem.qml \
    $$ROOT/qml/cover/CoverPage.qml \
    $$ROOT/qml/cover/CoverConfirm.qml \
    $$ROOT/qml/cover/CoverActionList1.qml \
    $$ROOT/qml/cover/CoverActionList2.qml \
    $$ROOT/qml/cover/CoverLabel.qml \
    $$ROOT/qml/cover/FontPreviewCover.qml \
    $$ROOT/qml/pages/ConfirmPage.qml \
    $$ROOT/qml/pages/DensityPage.qml \
    $$ROOT/qml/pages/MainPage.qml \
    $$ROOT/qml/pages/OptionsPage.qml \
    $$ROOT/qml/pages/RestorePage.qml \
    $$ROOT/qml/pages/RestoreDDPage.qml \
    $$ROOT/qml/pages/WelcomePage.qml \
    $$ROOT/qml/pages/AboutPage.qml \
    $$ROOT/qml/pages/GuidePage.qml \
    $$ROOT/qml/pages/RecoveryPage.qml \
    $$ROOT/rpm/* \
    $$ROOT/sailfishos-uithemer.desktop

TRANSLATIONS += $$files($$ROOT/translations/*.ts)

# --- non-source asset installs ---
# 2.6.0: legacy tps/ + scripts/ shells were retired (the daemon owns
# every privileged op now). Only the systemd unit files, image assets
# and app icons need explicit install rules.

service.files = $$files($$ROOT/service/*)
service.path  = /usr/share/$$TARGET/service

images.files = $$files($$ROOT/images/*)
images.path  = /usr/share/$$TARGET/images

appicons.files = $$files($$ROOT/appicons/*)
appicons.path  = /usr/share/icons/hicolor/

# --- daemon plumbing assets ---
# Shipped here (not in daemon.pro) so the GUI subproject owns the package
# data layout. The daemon binary itself installs through daemon.pro.
dbusconf.files    = $$ROOT/dbus/org.uithemer.UiThemer1.conf
dbusconf.path     = /etc/dbus-1/system.d

dbusservice.files = $$ROOT/dbus/org.uithemer.UiThemer1.service
dbusservice.path  = /usr/share/dbus-1/system-services

dbusxml.files = \
    $$ROOT/dbus/org.uithemer.UiThemer1.Themes.xml \
    $$ROOT/dbus/org.uithemer.UiThemer1.Packs.xml \
    $$ROOT/dbus/org.uithemer.UiThemer1.SystemServices.xml
dbusxml.path  = /usr/share/dbus-1/interfaces

polkit.files  = $$ROOT/polkit/org.uithemer.policy
polkit.path   = /usr/share/polkit-1/actions

# helperd.service ships under service/ alongside the other systemd
# units; %post moves it into /etc/systemd/system/ together with the
# autoupdate / icond units.

INSTALLS += service images appicons \
            dbusconf dbusservice dbusxml polkit
