import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0
import harbour.muoto 1.0
import Nemo.DBus 2.0
import "themepacklistview"

SilicaListView {
    id: themesView
    anchors.fill: parent
    enabled: !settings.isRunning
    opacity: settings.isRunning ? 0.2 : 1.0

    property bool tabActive: true

    property int _pendingOps: 0
    property bool _waitForFinalise: false
    property string _pendingIconPack: ""
    property bool _pendingIconOverlay: false
    property bool _pendingIconRestore: false
    property bool _uninstallAfterIconRestore: false
    property int _uninstallPackIndex: -1

    function _commitPendingIconApply() {
        if (_pendingIconPack !== "") {
            settings.activeIconPack = _pendingIconPack
            settings.iconOverlay = _pendingIconOverlay
            _pendingIconPack = ""
        }
    }
    function _commitPendingIconRestore() {
        if (_pendingIconRestore) {
            settings.deactivateIcon()
            settings.iconOverlay = false
            _pendingIconRestore = false
        }
    }

    function _armApply(nOps) {
        _waitForFinalise = true
        _pendingOps = nOps
    }
    function _opDone() {
        if (!_waitForFinalise)
            return
        if (_pendingOps > 0)
            _pendingOps -= 1
        if (_pendingOps === 0) {
            _waitForFinalise = false
            _finalise()
        }
    }
    function _finalise() {
        settings.isRunning = false
        notification.previewBody = qsTr("Settings applied.")
        notification.publish()
        settings.syncToDisk()
        if (settings.homeRefresh === true)
            lipstickRestartTimer.start()
    }

    RemorsePopup { id: remorsepopup }
    ThemePack { id: themepack }
    Notification { id: notification }

    Timer {
        id: lipstickRestartTimer
        interval: 250
        repeat: false
        onTriggered: themepack.restartHomescreen()
    }

    Connections {
        target: Helper
        onIconsApplied: {
            themesView._commitPendingIconApply()
            themesView._opDone()
        }
        onIconsRestored: {
            themesView._commitPendingIconRestore()
            if (themesView._uninstallAfterIconRestore) {
                var idx = themesView._uninstallPackIndex
                themesView._uninstallAfterIconRestore = false
                themesView._uninstallPackIndex = -1
                if (idx >= 0)
                    themepackmodel.uninstall(idx)
            }
            themesView._opDone()
        }
        onError: {
            notification.previewBody = message.length
                ? message
                : qsTr("Operation failed")
            if (op === "ApplyIcons") {
                themesView._pendingIconPack = ""
                themesView._opDone()
            } else if (op === "RestoreIcons") {
                themesView._pendingIconRestore = false
                themesView._uninstallAfterIconRestore = false
                themesView._uninstallPackIndex = -1
                themesView._opDone()
            }
        }
    }

    ThemePackModel {
        function notifyDone() {
            settings.isRunning = false
            notification.publish()
        }

        id: themepackmodel
        onThemeApplied: themesView._opDone()
        onThemeRestored: themesView._opDone()
        onUninstallCompleted: notifyDone()
    }

    Timer {
        id: timer
        interval: 10000
        repeat: true
        running: !settings.isRunning && Qt.application.state === Qt.ApplicationActive
        onTriggered: themepackmodel.reloadAll()
    }

    DBusInterface {
        id: openStore
        service: 'harbour.storeman.service'
        path: '/harbour/storeman/service'
        iface: 'harbour.storeman.service'
    }

    PullDownMenu {
        flickable: themesView
        enabled: tabActive

        MuotoAboutMenuItem { }

        MuotoRestartHomescreenMenuItem {
            remorsePopup: remorsepopup
            themePack: themepack
        }

        MenuItem {
            text: qsTr("Support Muoto")
            onClicked: app.showSupportDialog()
        }

        MenuItem {
            text: qsTr("Restart first run wizard")
            onClicked: {
                settings.wizardDone = false
                pageStack.replaceAbove(null, Qt.resolvedUrl("../pages/WelcomePage.qml"))
            }
        }

        MenuItem {
            visible: themepack.hasStoremanInstalled()
            text: qsTr("Download more themes")
            onClicked: openStore.call('openPage',
                ['SearchPage', { initialSearch: 'themepack' }])
        }

        MenuItem {
            text: qsTr("Restore theme")

            onClicked: {
                var dlgrestore = pageStack.push(Qt.resolvedUrl("../pages/RestorePage.qml"),
                                                { "settings": settings })

                dlgrestore.accepted.connect(function() {
                    var nOps = (dlgrestore.restoreIcons ? 1 : 0)
                             + (dlgrestore.restoreFonts ? 1 : 0)
                    if (nOps === 0)
                        return

                    settings.isRunning = true
                    themesView._armApply(nOps)
                    settings.syncToDisk()

                    if (dlgrestore.restoreFonts) {
                        settings.deactivateFont()
                        themepackmodel.restoreTheme(dlgrestore.restoreFonts)
                    }
                    if (dlgrestore.restoreIcons) {
                        themesView._pendingIconRestore = true
                        Helper.restoreIcons()
                    }
                })
            }
        }
    }

    model: themepackmodel

    delegate: ThemePackItem {
        iconInstalled: model.packName === settings.activeIconPack
        fontInstalled: model.packName === settings.activeFontPack

        onClicked: {
            timer.stop()
            app.coverMode = "confirmDialog"
            var dlgconfirm = pageStack.push(Qt.resolvedUrl("../pages/ConfirmPage.qml"), {
                "settings": settings,
                "themePackModel": themepackmodel,
                "themePackIndex": index
            })

            dlgconfirm.accepted.connect(function() {
                var wantsIcons = dlgconfirm.iconsSelected
                    || dlgconfirm.iconOverlaySelected
                var nOps = (wantsIcons ? 1 : 0)
                         + (dlgconfirm.fontsSelected ? 1 : 0)
                if (nOps === 0)
                    return

                settings.isRunning = true
                themesView._armApply(nOps)
                settings.syncToDisk()

                if (dlgconfirm.fontsSelected) {
                    settings.activeFontPack = model.packName
                    themepackmodel.applyTheme(index,
                        dlgconfirm.fontsSelected,
                        dlgconfirm.selectedFont)
                }
                if (wantsIcons) {
                    themesView._pendingIconPack = model.packName
                    themesView._pendingIconOverlay = dlgconfirm.iconOverlaySelected
                    Helper.applyIcons(model.packName,
                        dlgconfirm.iconsSelected,
                        dlgconfirm.iconOverlaySelected)
                }
            })
        }

        onUninstallRequested: {
            remorsepopup.execute(
                qsTr("Uninstalling %1").arg(model.packName),
                function() {
                    settings.isRunning = true

                    if (iconInstalled) {
                        themesView._pendingIconRestore = true
                        themesView._uninstallAfterIconRestore = true
                        themesView._uninstallPackIndex = index
                        Helper.restoreIcons()
                    } else {
                        themepackmodel.uninstall(index)
                    }

                    if (fontInstalled)
                        settings.deactivateFont()
                })
        }
    }

    ViewPlaceholder {
        enabled: themesView.count == 0
        text: qsTr("No themes yet")
        hintText: qsTr("Install a compatible theme first")
    }

    Item {
        width: parent.width
        height: Theme.paddingLarge
    }

    VerticalScrollDecorator { }
}
