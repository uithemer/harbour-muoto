import Nemo.Notifications 1.0
import QtQuick 2.0
import Sailfish.Silica 1.0
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
    property string _pendingFontPack: ""
    property bool _pendingFontRestore: false
    property bool _uninstallAfterIconRestore: false
    property int _uninstallPackIndex: -1
    // Font + icon share icon-ops.lock; with async fonts, start icons only
    // after fonts finish (same order as when applyFromPack blocked the UI).
    property string _deferredIconPack: ""
    property bool _deferredIconRunPack: false
    property bool _deferredIconOverlay: false
    property bool _deferredIconRestore: false

    function _clearDeferredIcons() {
        _deferredIconPack = ""
        _deferredIconRunPack = false
        _deferredIconOverlay = false
        _deferredIconRestore = false
    }

    function _startDeferredIcons() {
        if (_deferredIconRestore) {
            _deferredIconRestore = false
            settings.dynamicClockEnabled = false
            settings.dynamicCalendarEnabled = false
            _pendingIconRestore = true
            Helper.restoreIcons()
            return
        }
        if (_deferredIconPack !== "") {
            var pack = _deferredIconPack
            var runPack = _deferredIconRunPack
            var overlay = _deferredIconOverlay
            _deferredIconPack = ""
            _deferredIconRunPack = false
            _deferredIconOverlay = false
            _pendingIconPack = pack
            _pendingIconOverlay = overlay
            Helper.applyIcons(pack, runPack, overlay)
        }
    }

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
    function _commitPendingFontApply() {
        if (_pendingFontPack !== "") {
            settings.activeFontPack = _pendingFontPack
            _pendingFontPack = ""
        }
    }
    function _commitPendingFontRestore() {
        if (_pendingFontRestore) {
            settings.deactivateFont()
            _pendingFontRestore = false
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
    function beginManualThemeWork(progressBody) {
        app.showProgressNotification(
            qsTr("Applying theme"),
            progressBody,
            Notification.ProgressIndeterminate)
    }

    function _abortThemeWork(errMsg) {
        _waitForFinalise = false
        _pendingOps = 0
        _pendingIconPack = ""
        _pendingFontPack = ""
        _pendingIconRestore = false
        _pendingFontRestore = false
        _uninstallAfterIconRestore = false
        _uninstallPackIndex = -1
        _clearDeferredIcons()
        settings.isRunning = false
        app.showHelperError(errMsg)
    }

    function _finalise() {
        settings.isRunning = false
        app.showToast(qsTr("Settings applied."))
        settings.syncToDisk()
        if (settings.homeRefresh === true)
            lipstickRestartTimer.start()
    }

    RemorsePopup { id: remorsepopup }
    ThemePack { id: themepack }

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
            if (op === "ApplyIcons") {
                themesView._pendingIconPack = ""
                themesView._abortThemeWork(message)
            } else if (op === "RestoreIcons") {
                themesView._pendingIconRestore = false
                themesView._uninstallAfterIconRestore = false
                themesView._uninstallPackIndex = -1
                themesView._abortThemeWork(message)
            }
        }
    }

    ThemePackModel {
        function notifyDone() {
            settings.isRunning = false
            app.showToast(qsTr("Settings applied."))
        }

        id: themepackmodel
        onThemeApplied: {
            themesView._commitPendingFontApply()
            themesView._opDone()
            themesView._startDeferredIcons()
        }
        onThemeApplyFailed: themesView._abortThemeWork(message)
        onThemeRestored: {
            themesView._commitPendingFontRestore()
            themesView._opDone()
            themesView._startDeferredIcons()
        }
        onThemeRestoreFailed: themesView._abortThemeWork(message)
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
        enabled: tabActive && !settings.isRunning

        MuotoAboutMenuItem { }

        MuotoRestartHomescreenMenuItem {
            remorsePopup: remorsepopup
            themePack: themepack
        }

        MenuItem {
            // debug
            visible: false
            text: "Support Muoto"
            onClicked: app.showSupportDialog()
        }

        MenuItem {
            // debug
            visible: false
            text: "Restart first run wizard"
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
                    themesView.beginManualThemeWork(qsTr("Restoring theme…"))
                    themesView._armApply(nOps)
                    settings.syncToDisk()

                    if (dlgrestore.restoreFonts) {
                        themesView._pendingFontRestore = true
                        if (dlgrestore.restoreIcons) {
                            // Defer icons until fonts release icon-ops.lock.
                            themesView._deferredIconRestore = true
                        }
                        themepackmodel.restoreTheme(dlgrestore.restoreFonts)
                    } else if (dlgrestore.restoreIcons) {
                        settings.dynamicClockEnabled = false
                        settings.dynamicCalendarEnabled = false
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
                themesView.beginManualThemeWork(qsTr("Applying theme…"))
                themesView._armApply(nOps)
                settings.syncToDisk()

                if (dlgconfirm.fontsSelected) {
                    themesView._pendingFontPack = model.packName
                    if (wantsIcons) {
                        // Defer icons until fonts release icon-ops.lock.
                        themesView._deferredIconPack = model.packName
                        themesView._deferredIconRunPack = dlgconfirm.iconsSelected
                        themesView._deferredIconOverlay = dlgconfirm.iconOverlaySelected
                    }
                    themepackmodel.applyTheme(index,
                        dlgconfirm.fontsSelected,
                        dlgconfirm.selectedFont)
                } else if (wantsIcons) {
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
                    themesView.beginManualThemeWork(qsTr("Uninstalling theme…"))

                    if (iconInstalled) {
                        settings.dynamicClockEnabled = false
                        settings.dynamicCalendarEnabled = false
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
        hintText: qsTr("Install a compatible theme to start")
    }

    Item {
        width: parent.width
        height: Theme.paddingLarge
    }

    VerticalScrollDecorator { }
}
