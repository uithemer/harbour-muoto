import QtQuick 2.0
import Sailfish.Silica 1.0
import "../common"
import "../components"

Dialog
{
    property Settings settings
    property alias restoreIcons: itsrestoreicons.checked
    property alias restoreFonts: itsrestorefonts.checked

    id: dlgrestore
    focus: true
    canAccept: itsrestoreicons.checked || itsrestorefonts.checked
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
                id: itsrestoreicons
                automaticCheck: true
                text: qsTr("Default icons")
                checked: true

                onClicked: {
                    if(!itsrestoreicons.checked && !itsrestorefonts.checked)
                        dlgrestore.canAccept = false
                    else
                        dlgrestore.canAccept = true
                }
            }

            IconTextSwitch {
                id: itsrestorefonts
                automaticCheck: true
                text: qsTr("Default fonts")
                checked: true

                onClicked: {
                    if(!itsrestoreicons.checked && !itsrestorefonts.checked)
                        dlgrestore.canAccept = false
                    else
                        dlgrestore.canAccept = true
                }
            }

            }
            } // grid

            LabelText {
                visible: settings.guimode === 0 ? false : true
                text: "<br>" + qsTr("Remember to restart the homescreen right after.")
            }

            TextSwitch {
                visible: settings.guimode === 0 ? false : true
                text: qsTr("Restart homescreen")
                checked: settings.homeRefresh
                onCheckedChanged: {
                    settings.homeRefresh = checked;
                }
            }

            LabelText {
                visible: settings.guimode === 0
                text: "<br>" + qsTr("After confirming, your device will restart. Your currently opened apps will be closed.")
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }
        }
    }
}
