import Nemo.Notifications 1.0
import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0

// Shared apply/restore orchestration (busy state, Helper, font/icon lock order).
Item {
    id: themeWork

    property bool reloadActive: true

    property int _pendingOps: 0
    property bool _waitForFinalise: false
    property string _pendingIconPack: ""
    property bool _pendingIconOverlay: false
    property bool _pendingIconRestore: false
    property string _pendingFontPack: ""
    property string _pendingFontWeight: ""
    property bool _pendingFontRestore: false
    property bool _uninstallAfterIconRestore: false
    property int _uninstallPackIndex: -1
    property string _deferredIconPack: ""
    property bool _deferredIconRunPack: false
    property bool _deferredIconOverlay: false
    property bool _deferredIconRestore: false
    property bool _reapplyDynAfterIconRestore: false
    property bool _pendingDynClock: false
    property bool _pendingDynCalendar: false
    // Body text of the in-flight progress notification, kept so daemon
    // progress updates can republish it with a real completion ratio.
    property string _progressBody: ""

    readonly property alias themepackmodel: themepackmodel
    readonly property alias remorsePopup: remorsepopup
    readonly property alias themePack: themepack

    function packLabel(packId) {
        if (!packId || packId === "" || packId === "default")
            return qsTr("Stock")
        var pref = "harbour-themepack-"
        var dir = packId.indexOf(pref) === 0 ? packId : pref + packId
        var n = themepackmodel.readThemePackName(dir)
        if (n && n !== "")
            return n
        if (dir.indexOf(pref) === 0)
            return dir.substring(pref.length).replace(/-/g, " ")
        return dir
    }

    function indexForPackName(packName) {
        if (!packName || packName === "" || packName === "default")
            return -1
        var bare = packName
        var pref = "harbour-themepack-"
        if (bare.indexOf(pref) === 0)
            bare = bare.substring(pref.length)
        for (var i = 0; i < themepackmodel.rowCount(); ++i) {
            var pn = themepackmodel.packName(i)
            if (pn === packName || pn === bare
                    || pn === pref + bare
                    || pn.replace(pref, "") === bare)
                return i
        }
        return -1
    }

    function firstIconPackIndex() {
        for (var i = 0; i < themepackmodel.rowCount(); ++i) {
            if (themepackmodel.hasIcons(i))
                return i
        }
        return -1
    }

    function firstFontPackIndex() {
        for (var i = 0; i < themepackmodel.rowCount(); ++i) {
            if (themepackmodel.hasFont(i) || themepackmodel.hasFontNonLatin(i))
                return i
        }
        return -1
    }

    function _clearDeferredIcons() {
        _deferredIconPack = ""
        _deferredIconRunPack = false
        _deferredIconOverlay = false
        _deferredIconRestore = false
    }

    function _startDeferredIcons() {
        if (_deferredIconRestore) {
            _deferredIconRestore = false
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
            settings.activeFontWeight = _pendingFontWeight
            _pendingFontPack = ""
            _pendingFontWeight = ""
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
        launcherSettleTimer.stop()
    }

    // deferLauncher: wait for Lipstick's ~2s monitor holdback before toasting.
    function _opDone(deferLauncher) {
        if (!_waitForFinalise)
            return
        if (_pendingOps > 0)
            _pendingOps -= 1
        if (_pendingOps === 0) {
            _waitForFinalise = false
            if (deferLauncher) {
                app.showProgressNotification(
                    qsTr("Applying theme"),
                    qsTr("Updating homescreen…"),
                    Notification.ProgressIndeterminate)
                launcherSettleTimer.start()
            } else {
                _finalise()
            }
        }
    }

    function beginManualThemeWork(progressBody) {
        launcherSettleTimer.stop()
        _progressBody = progressBody
        app.showProgressNotification(
            qsTr("Applying theme"),
            progressBody,
            Notification.ProgressIndeterminate)
    }

    function _abortThemeWork(errMsg) {
        launcherSettleTimer.stop()
        _waitForFinalise = false
        _pendingOps = 0
        _pendingIconPack = ""
        _pendingFontPack = ""
        _pendingFontWeight = ""
        _pendingIconRestore = false
        _pendingFontRestore = false
        _uninstallAfterIconRestore = false
        _uninstallPackIndex = -1
        _reapplyDynAfterIconRestore = false
        _clearDeferredIcons()
        _progressBody = ""
        settings.isRunning = false
        app.showHelperError(errMsg)
    }

    function _finalise(message) {
        _progressBody = ""
        settings.isRunning = false
        app.showToast(message || qsTr("Theme updated."))
        settings.syncToDisk()
        if (settings.homeRefresh === true)
            lipstickRestartTimer.start()
    }

    function applyIconsOnly(pack, runPack, overlay) {
        settings.isRunning = true
        beginManualThemeWork(qsTr("Applying icons…"))
        _armApply(1)
        settings.syncToDisk()
        _pendingIconPack = pack
        _pendingIconOverlay = overlay
        Helper.applyIcons(pack, runPack, overlay)
    }

    function applyFontOnly(packIndex, weight, packName) {
        settings.isRunning = true
        beginManualThemeWork(qsTr("Applying fonts…"))
        _armApply(1)
        settings.syncToDisk()
        _pendingFontPack = packName
        _pendingFontWeight = weight
        themepackmodel.applyTheme(packIndex, true, weight)
    }

    function restoreDpi(dpr, iconSize) {
        if (!dpr && !iconSize)
            return
        settings.isRunning = true
        themepackmodel.restoreDpi(dpr, iconSize)
    }

    function finishDensityApply() {
        _finalise(qsTr("Display settings updated."))
    }

    function uninstallPack(index) {
        if (index < 0 || index >= themepackmodel.rowCount())
            return

        settings.isRunning = true
        beginManualThemeWork(qsTr("Uninstalling theme…"))

        var iconInstalled = themeWork.indexForPackName(settings.activeIconPack) === index
        var fontInstalled = themeWork.indexForPackName(settings.activeFontPack) === index

        if (iconInstalled) {
            settings.dynamicClockEnabled = false
            settings.dynamicCalendarEnabled = false
            _pendingIconRestore = true
            _uninstallAfterIconRestore = true
            _uninstallPackIndex = index
            Helper.restoreIcons()
        } else {
            themepackmodel.uninstall(index)
        }

        if (fontInstalled)
            settings.deactivateFont()
    }

    function beginRestore(restoreIcons, restoreFonts) {
        var nOps = (restoreIcons ? 1 : 0) + (restoreFonts ? 1 : 0)
        if (nOps === 0)
            return

        settings.isRunning = true
        beginManualThemeWork(qsTr("Restoring theme…"))
        _armApply(nOps)
        if (restoreIcons) {
            _reapplyDynAfterIconRestore = true
            _pendingDynClock = settings.dynamicClockEnabled
            _pendingDynCalendar = settings.dynamicCalendarEnabled
        }
        settings.syncToDisk()

        if (restoreFonts) {
            _pendingFontRestore = true
            if (restoreIcons)
                _deferredIconRestore = true
            themepackmodel.restoreTheme(restoreFonts)
        } else if (restoreIcons) {
            _pendingIconRestore = true
            Helper.restoreIcons()
        }
    }

    RemorsePopup { id: remorsepopup }

    ThemePack { id: themepack }

    Timer {
        id: lipstickRestartTimer
        interval: 250
        repeat: false
        onTriggered: themepack.restartHomescreen()
    }

    // Lipstick LauncherMonitor holdback is 2s; wait a bit longer before toast.
    Timer {
        id: launcherSettleTimer
        interval: 2500
        repeat: false
        onTriggered: themeWork._finalise()
    }

    Connections {
        target: Helper
        onIconProgress: {
            if (themeWork._progressBody === "")
                return
            if (total <= 0) {
                app.showProgressNotification(
                    qsTr("Applying theme"),
                    qsTr("Waiting…"),
                    Notification.ProgressIndeterminate)
                return
            }
            app.showProgressNotification(qsTr("Applying theme"),
                                         themeWork._progressBody,
                                         done / total)
        }
        onIconsApplied: {
            themeWork._commitPendingIconApply()
            themeWork._opDone(true)
        }
        onIconsRestored: {
            themeWork._commitPendingIconRestore()
            if (themeWork._reapplyDynAfterIconRestore) {
                themeWork._reapplyDynAfterIconRestore = false
                // Pulse when QML still reads the old value so dconf watches fire
                // after RestoreIcons (or an older daemon) forced the keys off.
                var clockOn = themeWork._pendingDynClock
                var calOn = themeWork._pendingDynCalendar
                if (settings.dynamicClockEnabled === clockOn)
                    settings.dynamicClockEnabled = !clockOn
                settings.dynamicClockEnabled = clockOn
                if (settings.dynamicCalendarEnabled === calOn)
                    settings.dynamicCalendarEnabled = !calOn
                settings.dynamicCalendarEnabled = calOn
            }
            if (themeWork._uninstallAfterIconRestore) {
                var idx = themeWork._uninstallPackIndex
                themeWork._uninstallAfterIconRestore = false
                themeWork._uninstallPackIndex = -1
                if (idx >= 0)
                    themepackmodel.uninstall(idx)
            }
            themeWork._opDone(true)
        }
        onError: {
            if (op === "ApplyIcons") {
                themeWork._pendingIconPack = ""
                themeWork._abortThemeWork(message)
            } else if (op === "RestoreIcons") {
                themeWork._pendingIconRestore = false
                themeWork._uninstallAfterIconRestore = false
                themeWork._uninstallPackIndex = -1
                themeWork._abortThemeWork(message)
            } else if (op === "UninstallPack") {
                themeWork._uninstallAfterIconRestore = false
                themeWork._uninstallPackIndex = -1
                themeWork._abortThemeWork(message)
            }
        }
    }

    ThemePackModel {
        id: themepackmodel
        onThemeApplied: {
            themeWork._commitPendingFontApply()
            themeWork._opDone(false)
            themeWork._startDeferredIcons()
        }
        onThemeApplyFailed: themeWork._abortThemeWork(message)
        onThemeRestored: {
            themeWork._commitPendingFontRestore()
            themeWork._opDone(false)
            themeWork._startDeferredIcons()
        }
        onThemeRestoreFailed: themeWork._abortThemeWork(message)
        onUninstallCompleted: {
            settings.isRunning = false
            app.showToast(qsTr("Theme removed."))
        }
        onDpiRestored: themeWork.finishDensityApply()
    }

    Timer {
        id: reloadTimer
        interval: 10000
        repeat: true
        running: themeWork.reloadActive && !settings.isRunning
                 && Qt.application.state === Qt.ApplicationActive
        onTriggered: themepackmodel.reloadAll()
    }
}
