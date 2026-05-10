import QtQuick 2.0
import Sailfish.Silica 1.0
import "../common"
import "../components"

Dialog
{
    property Settings settings
    property alias restoreDPR: itsrestoredpr.checked
    property alias restoreIconSize: itsrestoreis.checked

    id: dlgrestore
    focus: true
    canAccept: itsrestoredpr.checked || itsrestoreis.checked

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

    DialogHeader {
        id: header
        dialog: dlgrestore
        acceptText: qsTr("Restore")
        cancelText: qsTr("Cancel")
    }

    SilicaFlickable
    {
        id: flickable
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        contentHeight: column.height
        enabled: !settings.isRunning
        opacity: settings.isRunning ? 0.2 : 1.0

        VerticalScrollDecorator { }

        Column
        {
            id: column
            width: parent.width

            ConfirmHeader { text: qsTr("Restore") }

            IconTextSwitch {
                id: itsrestoredpr
                automaticCheck: true
                text: qsTr("Default device pixel ratio")
                checked: true

                onClicked: {
                    dlgrestore.canAccept = itsrestoredpr.checked || itsrestoreis.checked
                }
            }

            IconTextSwitch {
                id: itsrestoreis
                automaticCheck: true
                text: qsTr("Default icon size")
                checked: true

                onClicked: {
                    dlgrestore.canAccept = itsrestoredpr.checked || itsrestoreis.checked
                }
            }

            LabelText {
                text: "<br>" + qsTr("Remember to restart the homescreen right after.")
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
