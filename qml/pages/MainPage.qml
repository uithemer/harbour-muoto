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

    ThemePackModel {
                function applyDone() {
                    notifyDone();
                    if(settings.homeRefresh === true) {
                        themepack.restartHomescreen();
                        console.log("homescreen restart");
                    } else
                        console.log("no homescreen restart");
                }
                function notifyDone() {
                    settings.isRunning = false;
                    notification.publish();
                }

                id: themepackmodel
                onThemeApplied: applyDone()
                onThemeRestored: applyDone()
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

        if (event.key === Qt.Key_D && settings.showDensity === true) {
            pageStack.push(Qt.resolvedUrl("DensityPage.qml"));
            event.accepted = true;
        }

        if (event.key === Qt.Key_O) {
            pageStack.push(Qt.resolvedUrl("OptionsPage.qml"));
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
                text: qsTr("Options")
                onClicked: pageStack.push(Qt.resolvedUrl("OptionsPage.qml"))
            }

            MenuItem {
                text: qsTr("Display density")
                visible: settings.showDensity
                onClicked: pageStack.push(Qt.resolvedUrl("DensityPage.qml"))
            }

            MenuItem {
                text: qsTr("Restore theme")

                onClicked: {
                    var dlgrestore = pageStack.push("RestorePage.qml", { "settings": settings });

                    dlgrestore.accepted.connect(function() {
                        settings.isRunning = true;

                        // Same ordering rule as the apply path: write dconf
                        // BEFORE the synchronous restoreTheme call so the
                        // cover sees activeFontPack="default" by the time
                        // FontApplier::restored fires inside the call.
                        if(dlgrestore.restoreIcons) {
                            settings.deactivateIcon();
                            iconapplier.restoreIcons();
                        }

                        if(dlgrestore.restoreFonts) {
                            settings.deactivateFont();
                            themepackmodel.restoreTheme(dlgrestore.restoreFonts);
                        } else if(dlgrestore.restoreIcons) {
                            settings.isRunning = false;
                            notification.publish();
                            if(settings.homeRefresh === true)
                                themepack.restartHomescreen();
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
                var dlgconfirm = pageStack.push("ConfirmPage.qml", { "settings": settings, "themePackModel": themepackmodel, "themePackIndex": model.index });

                dlgconfirm.accepted.connect(function() {
                    settings.isRunning = true;

                    // Write dconf BEFORE the C++ apply calls. FontApplier is
                    // synchronous in 2.5.0, so themeApplied (which clears
                    // settings.isRunning via MainPage.applyDone) fires
                    // inside applyTheme(...). If activeFontPack were written
                    // afterwards the cover would re-render once with the
                    // stale value and leave the font CoverLabel empty.
                    if(dlgconfirm.iconsSelected) {
                        settings.activeIconPack = model.packName;
                        iconapplier.applyIcons(model.packName, dlgconfirm.iconOverlaySelected);
                    }

                    if(dlgconfirm.fontsSelected) {
                        settings.activeFontPack = model.packName;
                        themepackmodel.applyTheme(model.index, dlgconfirm.fontsSelected, dlgconfirm.selectedFont)
                    } else if(dlgconfirm.iconsSelected) {
                        // Icons-only: applyIcons is synchronous, finalise here.
                        settings.isRunning = false;
                        notification.publish();
                        if(settings.homeRefresh === true)
                            themepack.restartHomescreen();
                    }
                });
            }

            onUninstallRequested: {
                remorseAction(qsTr("Uninstalling %1").arg(model.packName), function() {
                    settings.isRunning = true;

                    if(iconInstalled) {
                        // Restore originals BEFORE rpm removes the pack, otherwise
                        // every themed Icon= ends up pointing at a path that is
                        // about to vanish and Lipstick falls back to placeholders.
                        iconapplier.restoreIcons();
                        settings.deactivateIcon();
                    }

                    themepackmodel.uninstall(model.index);

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
