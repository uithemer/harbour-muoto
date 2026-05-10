import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.uithemer 1.0
import org.nemomobile.notifications 1.0
import "../components"

Page
{
    id: optionspage
    focus: true
    backNavigation: !settings.isRunning
    showNavigationIndicator: !settings.isRunning

    RemorsePopup { id: remorsepopup }
    BusyState { id: busyindicator }
    Notification { id: notification }

    Keys.onPressed: {
        handleKeyPressed(event);
    }

    function handleKeyPressed(event) {

        if (event.key === Qt.Key_Down) {
            flickable.flick(0, - optionspage.height);
            event.accepted = true;
        }

        if (event.key === Qt.Key_Up) {
            flickable.flick(0, optionspage.height);
            event.accepted = true;
        }

        if (event.key === Qt.Key_PageDown) {
            flickable.scrollToBottom();
            event.accepted = true;
        }

        if (event.key === Qt.Key_PageUp) {
            flickable.scrollToTop();
            event.accepted = true;
        }

        if (event.key === Qt.Key_Return) {
            if (remorsepopup.active)
            remorsepopup.trigger();
            event.accepted = true;
        }

        if (event.key === Qt.Key_B) {
            pageStack.navigateBack();
            event.accepted = true;
        }

        if (event.key === Qt.Key_C) {
            remorsepopup.cancel();
            event.accepted = true;
        }

        if (event.key === Qt.Key_H) {
            pageStack.replaceAbove(null, Qt.resolvedUrl("MainPage.qml"));
            event.accepted = true;
        }

        if (event.key === Qt.Key_D && settings.showDensity === true) {
            pageStack.replace(Qt.resolvedUrl("DensityPage.qml"));
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
            remorsepopup.execute(qsTr("Restarting homescreen"), function() {
                settings.isRunning = true;
                themepack.restartHomescreen();
            });
            event.accepted = true;
        }
    }

SilicaFlickable
{
    id: flickable
    anchors.fill: parent
    contentHeight: content.height
    enabled: !settings.isRunning
    opacity: settings.isRunning ? 0.2 : 1.0

    VerticalScrollDecorator { }

    ThemePack {
                id: themepack
                onServiceChanged: itsservicesu.busy = false;
            }

    ThemePackModel {
                function applyDone() {
                    notifyDone();
                }
                function notifyDone() {
                    settings.isRunning = false;
                    notification.publish();
                }

                id: themepackmodel
                onOcrRestored: applyDone()
                onThemeRecovered: {
                    applyDone();
                    if(settings.homeRefresh === true) {
                        themepack.restartHomescreen();
                        console.log("homescreen restart");
                    } else
                        console.log("no homescreen restart");
                }
            }

    PullDownMenu
    {
        MenuItem {
            text: qsTr("About UI Themer")
            onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
        }
        MenuItem {
            text: qsTr("Usage guide")
            onClicked: pageStack.push(Qt.resolvedUrl("GuidePage.qml"))
        }
        MenuItem {
            text: qsTr("Restart first run wizard")
            onClicked: {
                settings.wizardDone = false
                pageStack.replaceAbove(null, Qt.resolvedUrl("WelcomePage.qml"))
            }
        }
    }

    Column
    {
        id: content
        width: parent.width

        PageHeader { title: qsTr("Options") }

        Grid {
            width: parent.width
            columns: isLandscape ? 2 : 1

        Column
        {
            width: isLandscape ? parent.width/2 : parent.width

            SectionHeader { text: qsTr("Restart homescreen") }

            LabelText {
                text: qsTr("Restart the homescreen, to make your modifications effective. Your currently opened apps will be closed.")
            }

            LabelSpacer { }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Restart")
                onClicked: {
                    remorsepopup.execute(qsTr("Restarting homescreen"), function() {
                        settings.isRunning = true;
                        themepack.restartHomescreen();
                    });
                }
            }

            SectionHeader { text: qsTr("One-click restore") }

            LabelText {
                text: qsTr("UI Themer customizations must be reverted before performing a system update. With <i>One-click restore</i> you can automate this process and restore icons, fonts and display density settings with just one click.")
            }

            LabelSpacer { }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Restore")                
                onClicked: {
                    remorsepopup.execute(qsTr("Restoring"), function() {
                        settings.isRunning = true;
                        themepackmodel.ocr();
                        settings.deactivateIcon();
                        settings.deactivateFont();
                    });
                }
            }

            IconTextSwitch {
                id: itsservicesu
                automaticCheck: true
                text: qsTr("Run before system updates")
                description: qsTr("Restore the default icons, fonts and display density settings before performing a system update, so you don't need to manually do it.")
                checked: settings.servicesu
                onClicked: {
                    itsservicesu.busy = true;
                    if (!settings.servicesu) {
                        themepack.enableservicesu();
                        settings.servicesu = true;
                    } else {
                        themepack.disableservicesu();
                        settings.servicesu = false;
                    }
                }
            }

            SectionHeader { text: qsTr("Icon updater") }

            ComboBox {
                function applyUpdater(setting, hours) {
                    settings.autoUpdate = setting;

                    if(setting === 0)
                        themepack.disableserviceautoupdate();
                    else
                        themepack.applyHours(hours);
                }

                function applyDaily() {
                    var dialog = pageStack.push("Sailfish.Silica.TimePickerDialog", { hourMode: DateTime.TwentyFourHours });
                    dialog.accepted.connect(function() { cbxupdate.applyUpdater(7, dialog.timeText); });
                }

                id: cbxupdate
                width: parent.width
                label: qsTr("Update icons")
                description: qsTr("Everytime an app is updated, you need to re-apply the theme in order to get the custom icon back. <i>Icon updater</i> will automate this process, enabling automatic update of icons at a given time.")
                currentIndex: settings.autoUpdate

                menu: ContextMenu {
                    MenuItem { text: qsTr("Disabled"); onClicked: cbxupdate.applyUpdater(0) }
                    MenuItem { text: qsTr("30 minutes"); onClicked: cbxupdate.applyUpdater(1, 30) }
                    MenuItem { text: qsTr("1 hour"); onClicked: cbxupdate.applyUpdater(2, 1) }
                    MenuItem { text: qsTr("2 hours"); onClicked: cbxupdate.applyUpdater(3, 2) }
                    MenuItem { text: qsTr("3 hours"); onClicked: cbxupdate.applyUpdater(4, 3) }
                    MenuItem { text: qsTr("6 hours"); onClicked: cbxupdate.applyUpdater(5, 6) }
                    MenuItem { text: qsTr("12 hours"); onClicked: cbxupdate.applyUpdater(6, 12) }
                    MenuItem { text: qsTr("Daily"); onClicked: cbxupdate.applyDaily(); }
                }
            }

            SectionHeader { text: qsTr("Cover") }

            ComboBox {
                function saveCoverAction(action) {
                    settings.coverAction1 = action;
                }

                id: cbxca1
                width: parent.width
                label: qsTr("Cover action")
                description: qsTr("Choose the action to be shown on the UI Themer cover, for a quick access when the app is minimized on the homescreen.")
                currentIndex: settings.coverAction1

                menu: ContextMenu {
                    MenuItem { text: qsTr("refresh current theme"); onClicked: cbxca1.saveCoverAction(0) }
                    MenuItem { text: qsTr("restart homescreen"); onClicked: cbxca1.saveCoverAction(1) }
                    MenuItem { text: qsTr("one-click restore"); onClicked: cbxca1.saveCoverAction(2) }
                    MenuItem { text: qsTr("none"); onClicked: cbxca1.saveCoverAction(3) }
                }
            }

        }

        Column
        {
            width: isLandscape ? parent.width/2 : parent.width

            ComboBox {
                function saveCoverAction(action) {
                    settings.coverAction2 = action;
                }

                id: cbxca2
                enabled: settings.coverAction1 !== 3
                width: parent.width
                label: qsTr("Second cover action")
                description: qsTr("Optionally, you can choose to display a second action on the cover.")
                currentIndex: settings.coverAction2

                menu: ContextMenu {
                    MenuItem { text: qsTr("refresh current theme"); onClicked: cbxca2.saveCoverAction(0) }
                    MenuItem { text: qsTr("restart homescreen"); onClicked: cbxca2.saveCoverAction(1) }
                    MenuItem { text: qsTr("one-click restore"); onClicked: cbxca2.saveCoverAction(2) }
                    MenuItem { text: qsTr("none"); onClicked: cbxca2.saveCoverAction(3) }
                }
            }

        SectionHeader { text: qsTr("Recovery") }

        LabelText {
            text: qsTr("Here you can find advanced settings for UI Themer, e.g. reinstall default icons or fonts if you forget to revert to default theme before a system update or if the applying fails.")
        }

            LabelSpacer { }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Recovery")
            onClicked: {
                var dlgrecovery = pageStack.push("RecoveryPage.qml", { "settings": settings });
                dlgrecovery.accepted.connect(function() {
                    settings.isRunning = true;

                    // Mirror the ordering rule from MainPage's apply /
                    // restore callbacks: write dconf BEFORE the synchronous
                    // recoveryTheme call, otherwise FontApplier::restored
                    // (which clears settings.isRunning via themeRecovered)
                    // fires inside the call and the cover re-renders with a
                    // stale activeFontPack value.
                    if(dlgrecovery.reinstallIcons) {
                        settings.deactivateIcon();
                        iconapplier.restoreIcons();
                    }

                    if(dlgrecovery.reinstallFonts) {
                        settings.deactivateFont();
                        themepackmodel.recoveryTheme(dlgrecovery.reinstallFonts);
                    } else if(dlgrecovery.reinstallIcons) {
                        settings.isRunning = false;
                        notification.publish();
                    }
                });
            }
        }

        }

    }  // grid

        Item {
            width: parent.width
            height: Theme.paddingLarge
        }

    }

}

}
