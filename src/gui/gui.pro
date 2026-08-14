TARGET = harbour-muoto
TEMPLATE = app

CONFIG += sailfishapp c++11 link_pkgconfig
PKGCONFIG += mlite5 sailfishsilica glib-2.0

LIBS += -L../launcher -lmuoto-launcher

# qmake feature .prfs added via `CONFIG += <name>` are auto-loaded at the end
# of .pro processing (default_post.prf phase). That means any qml/desktop/icon
# install variables we set later in this file would be silently overwritten by
# sailfishapp.prf's defaults (qml.files = qml, desktop.files = $${TARGET}.desktop)
# resolved relative to src/gui/, where those files do not exist -- qmake then
# omits install_qml / install_desktop entirely and rpmbuild trips on
# `desktop-file-install ... /usr/share/applications/*.desktop`.
# Force the .prf to load NOW so our overrides further down are the last word.
load(sailfishapp)
# sailfishapp_i18n.prf is intentionally NOT loaded: it assumes the .ts files
# live at $${_PRO_FILE_PWD_}/translations/ (next to the .pro), which after the
# 2.6.0 subdirs refactor is wrong -- they're at $$ROOT/translations/. The prf
# would build lupdate / cp / lrelease paths like
#   /.../src/gui//home/.../translations/foo.ts
# (double absolute prefix) and silently fail to ship any .qm. We replicate
# its flow further down with proper $$ROOT-rooted paths.

QT += dbus concurrent

# Shared ops (IconApplier, DensityEnabler, file lock, spawner, manifest,
# desktopfile, imageutil, iconpreviewcache, themepack-ops). FontApplier
# and FontWeightModel stay GUI-only because they are unprivileged and
# only the QML side ever touches them.
include(../ops/ops.pri)

INCLUDEPATH += $$PWD $$PWD/../launcher $$PWD/../ops

HEADERS += \
    $$PWD/themepack.h \
    $$PWD/themepackmodel.h \
    $$PWD/fontweightmodel.h \
    $$PWD/fontcarouselmodel.h \
    $$PWD/fontapplier.h \
    $$PWD/iconpreviewprovider.h \
    $$PWD/fontsampleprovider.h \
    $$PWD/helperclient.h \
    $$PWD/launcherimageprovider.h

SOURCES += \
    $$PWD/harbour-muoto.cpp \
    $$PWD/themepack.cpp \
    $$PWD/themepackmodel.cpp \
    $$PWD/fontweightmodel.cpp \
    $$PWD/fontcarouselmodel.cpp \
    $$PWD/fontapplier.cpp \
    $$PWD/iconpreviewprovider.cpp \
    $$PWD/fontsampleprovider.cpp \
    $$PWD/helperclient.cpp \
    $$PWD/launcherimageprovider.cpp

ROOT = $$PWD/../..

include($$ROOT/libs/opal.pri)

OTHER_FILES += \
    $$ROOT/qml/harbour-muoto.qml \
    $$ROOT/qml/common/Settings.qml \
    $$ROOT/qml/components/BusyState.qml \
    $$ROOT/qml/components/HomeTile.qml \
    $$ROOT/qml/components/ThemeWork.qml \
    $$ROOT/qml/components/IconPackPreview.qml \
    $$ROOT/qml/components/IconPackCarouselTile.qml \
    $$ROOT/qml/components/StockLauncherIcons.qml \
    $$ROOT/qml/components/FontPackCarouselTile.qml \
    $$ROOT/qml/components/FontPreview.qml \
    $$ROOT/qml/components/FontWeightSwitch.qml \
    $$ROOT/qml/components/LabelSpacer.qml \
    $$ROOT/qml/components/MuotoButton.qml \
    $$ROOT/qml/components/MuotoHeaderLabel.qml \
    $$ROOT/qml/components/MuotoTextLabel.qml \
    $$ROOT/qml/components/HomescreenRestartSection.qml \
    $$ROOT/qml/components/homescreenRestart.js \
    $$ROOT/qml/components/MuotoNotification.qml \
    $$ROOT/qml/components/themepacklistview/ThemePackItem.qml \
    $$ROOT/qml/cover/CoverPage.qml \
    $$ROOT/qml/cover/CoverConfirm.qml \
    $$ROOT/qml/cover/CoverLabel.qml \
    $$ROOT/qml/cover/FontPreviewCover.qml \
    $$ROOT/qml/pages/ConfirmPage.qml \
    $$ROOT/qml/components/DynamicIconsTabContent.qml \
    $$ROOT/qml/components/MuotoAboutMenuItem.qml \
    $$ROOT/qml/components/MuotoRestartHomescreenMenuItem.qml \
    $$ROOT/qml/components/ThemesTabContent.qml \
    $$ROOT/qml/components/MuotoSupportDialog.qml \
    $$ROOT/qml/pages/MainPage.qml \
    $$ROOT/qml/pages/IconsConfigurePage.qml \
    $$ROOT/qml/pages/FontsConfigurePage.qml \
    $$ROOT/qml/pages/DensityPage.qml \
    $$ROOT/qml/pages/DynamicIconsPage.qml \
    $$ROOT/qml/pages/RestorePage.qml \
    $$ROOT/qml/pages/WelcomePage.qml \
    $$ROOT/qml/pages/AboutPage.qml \
    $$ROOT/rpm/* \
    $$ROOT/harbour-muoto.desktop

# --- translations (manual; replaces sailfishapp_i18n.prf) ---
# We don't use sailfishapp_i18n.prf (see comment near `load(sailfishapp)`).
# Instead replicate its lupdate -> cp -> lrelease -> install_qm flow with
# explicit $$ROOT-rooted paths so the .ts files at the project root are
# picked up correctly and the generated .qm files actually ship.
#
# Flow:
#   1. lupdate scans $$ROOT/src + $$ROOT/qml for tr() and updates every
#      .ts in $$ROOT/translations/ in place (matches the prf's behaviour
#      of refreshing .ts on every build).
#   2. The .ts files are copied into $$OUT_PWD/translations/ so a shadow
#      build (rpmbuild, sfdk) doesn't write .qm into the source tree.
#   3. lrelease compiles every .ts in that dir into a sibling .qm.
#   4. install_qm picks the .qm files up and drops them under
#      /usr/share/<TARGET>/translations.
TS_SOURCES   = $$files($$ROOT/translations/*.ts)
QM_OUT_DIR   = $$OUT_PWD/translations
TR_SCAN_DIRS = $$ROOT/src $$ROOT/qml

qm.path     = /usr/share/$${TARGET}/translations
qm.CONFIG  += no_check_exist
qm.commands = mkdir -p $$QM_OUT_DIR && \
              lupdate -noobsolete $$TR_SCAN_DIRS -ts $$TS_SOURCES && \
              cp -af $$TS_SOURCES $$QM_OUT_DIR/ && \
              lrelease -nounfinished $$QM_OUT_DIR/*.ts

# Build the matching .qm path for every .ts source (basename + .qm).
for(ts_path, TS_SOURCES) {
    bn       = $$basename(ts_path)
    bn_noext = $$section(bn, ".", 0, -2)
    qm.files += $$QM_OUT_DIR/$${bn_noext}.qm
}

# --- non-source asset installs ---
# 2.6.0: legacy tps/ + scripts/ shells were retired (the daemon owns
# every privileged op now). Only the systemd unit files, image assets
# and app icons need explicit install rules.

service.files = \
    $$ROOT/service/harbour-muoto-helperd.service \
    $$ROOT/service/harbour-muoto-update-icons.service \
    $$ROOT/service/harbour-muoto-oneshot-restore.service \
    $$ROOT/service/muoto-dbus-wait.sh
service.path  = /usr/share/$$TARGET/service

autobin.files = \
    $$ROOT/service/harbour-muoto-update-icons \
    $$ROOT/service/harbour-muoto-oneshot-restore \
    $$ROOT/service/harbour-muoto-migrate-bulk-icons
autobin.path = /usr/bin

upgrade_dropin.files = \
    $$ROOT/service/sailfish-upgrade-ui.service.d/muoto-oneshot-restore.conf
upgrade_dropin.path = /usr/share/$$TARGET/service/sailfish-upgrade-ui.service.d

userunit.files = \
    $$ROOT/service/systemd/user/harbour-muoto-install-listener.service \
    $$ROOT/service/harbour-muoto-launcher-icond.service
userunit.path = /usr/lib/systemd/user

# launcher-icons dir created in RPM %post

images.files = $$files($$ROOT/images/*)
images.path  = /usr/share/$$TARGET/images

# sailfishapp.prf assumes the .pro file lives at the project root and
# resolves qml/desktop/icon sources relative to it. After the 2.6.0
# split this .pro lives in src/gui/, so override the inherited defaults
# to point at the real, $$ROOT-relative locations. Without this qmake
# silently skips them and rpmbuild then trips on
#   desktop-file-install ... %{buildroot}/usr/share/applications/*.desktop
# because the glob has nothing to match. The QML tree and the per-size
# app icons would be missing too.

# QML tree at <root>/qml -> /usr/share/harbour-muoto/qml
qml.files = $$ROOT/qml

# .desktop at <root>/harbour-muoto.desktop -> /usr/share/applications
desktop.files = $$ROOT/harbour-muoto.desktop

# Replace sailfishapp.prf's default `icon` rule (which would look for
# harbour-muoto.png next to gui.pro) with explicit per-size rules
# pulling from <root>/appicons/<size>/apps/. The earlier `appicons` rule
# (non-recursive $$files() over a subdir-only tree) was a no-op, so app
# icons never made it into the buildroot.
INSTALLS  -= icon
icon.files =

icon86.files  = $$ROOT/appicons/86x86/apps/$${TARGET}.png
icon86.path   = /usr/share/icons/hicolor/86x86/apps
icon108.files = $$ROOT/appicons/108x108/apps/$${TARGET}.png
icon108.path  = /usr/share/icons/hicolor/108x108/apps
icon128.files = $$ROOT/appicons/128x128/apps/$${TARGET}.png
icon128.path  = /usr/share/icons/hicolor/128x128/apps
icon172.files = $$ROOT/appicons/172x172/apps/$${TARGET}.png
icon172.path  = /usr/share/icons/hicolor/172x172/apps
icon256.files = $$ROOT/appicons/256x256/apps/$${TARGET}.png
icon256.path  = /usr/share/icons/hicolor/256x256/apps

# --- daemon plumbing assets ---
# Shipped here (not in daemon.pro) so the GUI subproject owns the package
# data layout. The daemon binary itself installs through daemon.pro.
dbusconf.files    = $$ROOT/dbus/org.muoto.Muoto1.conf
dbusconf.path     = /etc/dbus-1/system.d

dbusservice.files = $$ROOT/dbus/org.muoto.Muoto1.service
dbusservice.path  = /usr/share/dbus-1/system-services

dbusxml.files = \
    $$ROOT/dbus/org.muoto.Muoto1.Themes.xml \
    $$ROOT/dbus/org.muoto.Muoto1.Packs.xml \
    $$ROOT/dbus/org.muoto.Launcher1.Themes.xml
dbusxml.path  = /usr/share/dbus-1/interfaces

sessiondbusconf.files = $$ROOT/dbus/org.muoto.Launcher1.conf
sessiondbusconf.path  = /etc/dbus-1/session.d

# No polkit hand-off. See dbus/org.muoto.Muoto1.conf
# for the new policy and src/daemon/helperservice.cpp for the
# matching no-op authorize() stubs.

# helperd.service ships under service/ and is moved into
# /etc/systemd/system/ by the RPM %post.

# `target`, `qml` and `desktop` are already on INSTALLS via sailfishapp.prf
# (we only redirected their .files above). Add the per-size icon* rules
# and our own asset rules here.
INSTALLS += service autobin upgrade_dropin userunit images \
            icon86 icon108 icon128 icon172 icon256 \
            dbusconf dbusservice dbusxml sessiondbusconf \
            qm
