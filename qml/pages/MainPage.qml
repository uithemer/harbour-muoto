import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0
import org.nemomobile.configuration 1.0
import harbour.uithemer 1.0
import Nemo.DBus 2.0
import "../components"
import "../components/themepacklistview"

Page
{
    id: mainpage
    focus: true

    RemorsePopup { id: remorsepopup }
    ThemePack { id: themepack }
    BusyState { id: busyindicator }
    Notification { id: notification }

    // 2.6.0 D-Bus refactor: every Helper.X call is now an async D-Bus
    // dispatch (~1-2 s round-trip), while FontApplier remains
    // in-process and synchronous. Before this gate the QML treated
    // Helper.applyIcons as if it had already finished and called
    // themepack.restartHomescreen() before the daemon had rewritten
    // any .desktop file — classic race that left lipstick half-themed.
    //
    // _pendingOps counts how many of the user's selected ops are
    // still in flight; _waitForFinalise is the "arm" flag so stray
    // signals (cover sync, uninstall's restoreIcons preamble) don't trigger our
    // finalise. _finalise() is the single chokepoint that clears the
    // busy indicator, fires the notification, and (optionally)
    // restarts lipstick — exactly once, exactly when every selected
    // op has reported completion.
    property int _pendingOps: 0
    property bool _waitForFinalise: false
    property string _pendingIconPack: ""
    property bool _pendingIconOverlay: false
    property bool _pendingIconRestore: false
    property bool _uninstallAfterIconRestore: false
    property int _uninstallPackIndex: -1

    function _commitPendingIconApply() {
        if(_pendingIconPack !== "") {
            settings.activeIconPack = _pendingIconPack;
            settings.iconOverlay = _pendingIconOverlay;
            _pendingIconPack = "";
        }
    }
    function _commitPendingIconRestore() {
        if(_pendingIconRestore) {
            settings.deactivateIcon();
            settings.iconOverlay = false;
            _pendingIconRestore = false;
        }
    }

    function _armApply(nOps) {
        _waitForFinalise = true;
        _pendingOps = nOps;
    }
    function _opDone() {
        if(!_waitForFinalise)
            return;
        if(_pendingOps > 0)
            _pendingOps -= 1;
        if(_pendingOps === 0) {
            _waitForFinalise = false;
            _finalise();
        }
    }
    function _finalise() {
        settings.isRunning = false;
        notification.publish();
        if(settings.homeRefresh === true)
            themepack.restartHomescreen();
    }

    // Connections.enabled needs QtQuick 2.4+; MainPage still imports
    // 2.0, so guard inside the slots via _opDone(), which already
    // short-circuits when _waitForFinalise is false. Stray signals
    // (cover sync, uninstall's restoreIcons preamble) just no-op here.
    Connections {
        target: Helper
        onIconsApplied: {
            mainpage._commitPendingIconApply();
            mainpage._opDone();
        }
        onIconsRestored: {
            mainpage._commitPendingIconRestore();
            if(mainpage._uninstallAfterIconRestore) {
                var idx = mainpage._uninstallPackIndex;
                mainpage._uninstallAfterIconRestore = false;
                mainpage._uninstallPackIndex = -1;
                if(idx >= 0)
                    themepackmodel.uninstall(idx);
            }
            mainpage._opDone();
        }
        onError: {
            if(op === "ApplyIcons") {
                mainpage._pendingIconPack = "";
                mainpage._opDone();
            } else if(op === "RestoreIcons") {
                mainpage._pendingIconRestore = false;
                mainpage._uninstallAfterIconRestore = false;
                mainpage._uninstallPackIndex = -1;
                mainpage._opDone();
            }
        }
    }

    ThemePackModel {
                function notifyDone() {
                    settings.isRunning = false;
                    notification.publish();
                }

                id: themepackmodel
                onThemeApplied: mainpage._opDone()
                onThemeRestored: mainpage._opDone()
                onUninstallCompleted: notifyDone()
            }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            app.coverMode = "mainPage"
        }
    }

    Timer {
        id: timer
        interval: 10000
        repeat: true
        running: !settings.isRunning && Qt.application.state === Qt.ApplicationActive
        onTriggered: themepackmodel.reloadAll()
    }

    Keys.onPressed: {
        handleKeyPressed(event);
    }

    function handleKeyPressed(event) {

        if (event.key === Qt.Key_Down) {
            themepacklistview.flick(0, - mainpage.height);
                    event.accepted = true;
        }

        if (event.key === Qt.Key_Up) {
            themepacklistview.flick(0, mainpage.height);
                    event.accepted = true;
        }

        if (event.key === Qt.Key_PageDown) {
            themepacklistview.scrollToBottom();
                    event.accepted = true;
        }

        if (event.key === Qt.Key_PageUp) {
            themepacklistview.scrollToTop();
                    event.accepted = true;
        }

        if (event.key === Qt.Key_Return) {
            if (remorsepopup.active)
            remorsepopup.trigger();
            event.accepted = true;
        }

        if (event.key === Qt.Key_C) {
            remorsepopup.cancel();
            event.accepted = true;
        }

        if (event.key === Qt.Key_D) {
            pageStack.push(Qt.resolvedUrl("DensityPage.qml"));
            event.accepted = true;
        }

        if (event.key === Qt.Key_G) {
            pageStack.push(Qt.resolvedUrl("GuidePage.qml"));
            event.accepted = true;
        }

        if (event.key === Qt.Key_W) {
            settings.wizardDone = false
            pageStack.replaceAbove(null, Qt.resolvedUrl("WelcomePage.qml"));
            event.accepted = true;
        }

        if (event.key === Qt.Key_A) {
            pageStack.push(Qt.resolvedUrl("AboutPage.qml"));
            event.accepted = true;
        }

        if (event.key === Qt.Key_R) {
            var dlgrestart = pageStack.push("RestartHSPage.qml");
            dlgrestart.accepted.connect(function() {
                    themepack.restartHomescreen();
                    console.log("homescreen restart");
            });
            event.accepted = true;
        }
    }

    DBusInterface {
        id: openStore
        service: 'harbour.storeman.service'
        path: '/harbour/storeman/service'
        iface: 'harbour.storeman.service'
    }

        SilicaListView {
        id: themepacklistview
        width: parent.width
        height: parent.height
        anchors.fill: parent
        enabled: !settings.isRunning
        opacity: settings.isRunning ? 0.2 : 1.0

        PullDownMenu {
            MenuItem {
                text: qsTr("Usage guide")
                onClicked: pageStack.push(Qt.resolvedUrl("GuidePage.qml"))
            }

            MenuItem {
                text: qsTr("About UI Themer")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }

            MenuItem {
                text: qsTr("Restart first run wizard")
                onClicked: {
                    settings.wizardDone = false
                    pageStack.replaceAbove(null, Qt.resolvedUrl("WelcomePage.qml"))
                }
            }

            MenuItem {
                text: qsTr("Restart homescreen (fallback)")
                onClicked: {
                    var dlgrestart = pageStack.push("RestartHSPage.qml");
                    dlgrestart.accepted.connect(function() {
                        themepack.restartHomescreen();
                    });
                }
            }

            MenuItem {
                text: qsTr("Display density")
                onClicked: pageStack.push(Qt.resolvedUrl("DensityPage.qml"))
            }

            MenuItem {
                text: qsTr("Restore theme")

                onClicked: {
                    var dlgrestore = pageStack.push("RestorePage.qml", { "settings": settings });

                    dlgrestore.accepted.connect(function() {
                        // Pre-count the user's selections so we can arm the
                        // finalise gate BEFORE kicking the first op — a
                        // synchronous FontApplier::restored would otherwise
                        // re-enter _opDone() before _pendingOps is set.
                        var nOps = (dlgrestore.restoreIcons ? 1 : 0)
                                 + (dlgrestore.restoreFonts ? 1 : 0);
                        if(nOps === 0)
                            return;

                        settings.isRunning = true;
                        mainpage._armApply(nOps);

                        // Same ordering rule as the apply path: write dconf
                        // BEFORE the synchronous restoreTheme call so the
                        // cover sees activeFontPack="default" by the time
                        // FontApplier::restored fires inside the call.
                        if(dlgrestore.restoreFonts) {
                            settings.deactivateFont();
                            themepackmodel.restoreTheme(dlgrestore.restoreFonts);
                        }
                        if(dlgrestore.restoreIcons) {
                            mainpage._pendingIconRestore = true;
                            Helper.restoreIcons();
                        }
                    });
                }
            }
        }

        header: Item {
            width: parent.width
            height: titlepageheader.height

            PageHeader { id: titlepageheader; title: qsTr("Themes") }

            IconButton {
                visible: themepack.hasStoremanInstalled()
                anchors.verticalCenter: titlepageheader.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: Theme.paddingMedium
                icon.width: Theme.iconSizeSmallPlus
                icon.height: Theme.iconSizeSmallPlus
                icon.color: Theme.highlightColor
                icon.source: isLightTheme ? "../../images/download.png" : "../../images/download-light.png"
                onClicked: openStore.call('openPage', ['SearchPage', {initialSearch: 'themepack'}])
            }
        }

        model: themepackmodel

        delegate: ThemePackItem {
            iconInstalled: model.packName === settings.activeIconPack
            fontInstalled: model.packName === settings.activeFontPack

            onClicked: {
                timer.stop()
                var dlgconfirm = pageStack.push("ConfirmPage.qml", { "settings": settings, "themePackModel": themepackmodel, "themePackIndex": index });

                dlgconfirm.accepted.connect(function() {
                    // Pre-count the user's selections so we can arm the
                    // finalise gate BEFORE kicking the first op —
                    // FontApplier is synchronous, so themepackmodel
                    // .applyTheme() would re-enter _opDone() before
                    // _pendingOps was set if we counted lazily.
                    var wantsIcons = dlgconfirm.iconsSelected || dlgconfirm.iconOverlaySelected
                    var nOps = (wantsIcons ? 1 : 0)
                             + (dlgconfirm.fontsSelected ? 1 : 0);
                    if(nOps === 0)
                        return;

                    settings.isRunning = true;
                    mainpage._armApply(nOps);

                    // Write dconf BEFORE the C++ apply calls. FontApplier is
                    // synchronous, so themeApplied fires inside
                    // applyTheme(...). If activeFontPack were written
                    // afterwards the cover would re-render once with the
                    // stale value and leave the font CoverLabel empty.
                    if(dlgconfirm.fontsSelected) {
                        settings.activeFontPack = model.packName;
                        themepackmodel.applyTheme(index, dlgconfirm.fontsSelected, dlgconfirm.selectedFont);
                    }
                    if(wantsIcons) {
                        mainpage._pendingIconPack = model.packName;
                        mainpage._pendingIconOverlay = dlgconfirm.iconOverlaySelected;
                        Helper.applyIcons(model.packName, dlgconfirm.iconsSelected, dlgconfirm.iconOverlaySelected);
                    }
                });
            }

            onUninstallRequested: {
                remorseAction(qsTr("Uninstalling %1").arg(model.packName), function() {
                    settings.isRunning = true;

                    if(iconInstalled) {
                        mainpage._pendingIconRestore = true;
                        mainpage._uninstallAfterIconRestore = true;
                        mainpage._uninstallPackIndex = index;
                        Helper.restoreIcons();
                    } else {
                        themepackmodel.uninstall(index);
                    }

                    if(fontInstalled)
                        settings.deactivateFont();
                });
            }
        }

        ViewPlaceholder {
            enabled: themepacklistview.count == 0
            text: qsTr("No themes yet")
            hintText: qsTr("Install a compatible theme first")
        }

        Item {
            width: parent.width
            height: Theme.paddingLarge
        }

        VerticalScrollDecorator { }
        }

}
