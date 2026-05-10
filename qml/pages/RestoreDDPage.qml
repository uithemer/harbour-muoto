import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.uithemer 1.0
import "../common"
import "../components"

Dialog
{
    property Settings settings
    property alias restoreDPR: itsrestoredpr.checked
    property alias restoreADPI: itsrestoreadpi.checked

    ThemePack { id: themepack }

    id: dlgrestore
    focus: true
    canAccept: itsrestoredpr.checked || ( itsrestoreadpi.checked && themepack.hasAndroidSupport )
    backNavigation: !settings.isRunning
    forwardNavigation: !settings.isRunning
    showNavigationIndicator: !settings.isRunning

    BusyState { id: busyindicator }

    Keys.onPressed: {
        handleKeyPressed(event);
    }

    function handleKeyPressed(event) {

        if (event.key === Qt.Key_Down) {
            flickable.flick(0, - dlgrestore.height);
            event.accepted = true;
        }

        if (event.key === Qt.Key_Up) {
            flickable.flick(0, dlgrestore.height);
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

        if ((event.key === Qt.Key_B) || (event.key === Qt.Key_C)) {
            pageStack.navigateBack();
            event.accepted = true;
        }

        if (event.key === Qt.Key_Return) {
            dlgrestore.accept();
            event.accepted = true;
        }

        if (event.key === Qt.Key_G) {
            pageStack.push(Qt.resolvedUrl("GuidePage.qml"));
            event.accepted = true;
        }
    }

    SilicaFlickable
    {
        id: flickable
        anchors.fill: parent
        contentHeight: column.height
        width: parent.width
        enabled: !settings.isRunning
        opacity: settings.isRunning ? 0.2 : 1.0

        VerticalScrollDecorator { }

        Column
        {
            id: column
            width: parent.width

            DialogHeader { id: header; acceptText: qsTr("Restore"); cancelText: qsTr("Cancel") }

            ConfirmHeader { text: qsTr("Restore") }

            Grid {
                width: parent.width
                columns: isLandscape ? 2 : 1

            Column
            {
                width: isLandscape ? parent.width/2 : parent.width

            IconTextSwitch {
                id: itsrestoredpr
                automaticCheck: true
                text: qsTr("Default device pixel ratio")
                checked: true

                onClicked: {
                    if(!itsrestoredpr.checked && !itsrestoreadpi.checked)
                        dlgrestore.canAccept = false
                    else
                        dlgrestore.canAccept = true
                }
            }

            }

            Column
            {
                width: isLandscape ? parent.width/2 : parent.width

            IconTextSwitch {
                id: itsrestoreadpi
                visible: themepack.hasAndroidSupport
                automaticCheck: true
                text: qsTr("Default Android DPI")
                checked: true

                onClicked: {
                    if(!itsrestoredpr.checked && !itsrestoreadpi.checked)
                        dlgrestore.canAccept = false
                    else
                        dlgrestore.canAccept = true
                }
            }

            }
            } // grid

            LabelText {
                text: "<br>" + qsTr("Remember to restart the homescreen right after.")
            }

            LabelText {
                visible: themepack.hasAndroidSupport && settings.isXA2
                text: qsTr("If you have an Xperia XA2 series device, a full restart may be needed to apply your Android settings.")
            }

            TextSwitch {
                text: qsTr("Restart homescreen")
                checked: settings.homeRefresh
                onCheckedChanged: {
                    settings.homeRefresh = checked;
                }
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }
        }
    }
}
