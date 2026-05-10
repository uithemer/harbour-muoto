import QtQuick 2.0
import Sailfish.Silica 1.0
import "../common"
import "../components"

Dialog
{
    property Settings settings
    property alias reinstallIcons: itsicons.checked
    property alias reinstallFonts: itsfonts.checked

    id: dlgrecovery
    focus: true
    canAccept: itsicons.checked || itsfonts.checked
    backNavigation: !settings.isRunning
    forwardNavigation: !settings.isRunning
    showNavigationIndicator: !settings.isRunning

    BusyState { id: busyindicator }

    Keys.onPressed: {
        handleKeyPressed(event);
    }

    function handleKeyPressed(event) {

        if (event.key === Qt.Key_Down) {
            flickable.flick(0, - dlgrecovery.height);
            event.accepted = true;
        }

        if (event.key === Qt.Key_Up) {
            flickable.flick(0, dlgrecovery.height);
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
            dlgrecovery.accept();
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

            DialogHeader { id: header; acceptText: qsTr("Continue"); cancelText: qsTr("Cancel") }

            ConfirmHeader { text: qsTr("Recovery") }

            Grid {
                width: parent.width
                columns: isLandscape ? 2 : 1

            Column
            {
                width: isLandscape ? parent.width/2 : parent.width

            IconTextSwitch {
                id: itsicons
                automaticCheck: true
                text: qsTr("Reinstall icons")
                description: qsTr("If any error occurs during themes applying/restoring, you can end up with messed up icons. From here, you can reinstall default Jolla app icons while, for thirdy party apps, you may need to reinstall/update apps to restore the default look.")
                checked: true

                onClicked: {
                    if(!itsicons.checked && !itsfonts.checked)
                        dlgrecovery.canAccept = false
                    else
                        dlgrecovery.canAccept = true
                }
            }

            }

            Column
            {
                width: isLandscape ? parent.width/2 : parent.width

            IconTextSwitch {
                id: itsfonts
                automaticCheck: true
                text: qsTr("Reinstall fonts")
                description: qsTr("Reinstall default fonts, if fonts applying/restoring fails.")
                checked: true

                onClicked: {
                    if(!itsicons.checked && !itsfonts.checked)
                        dlgrecovery.canAccept = false
                    else
                        dlgrecovery.canAccept = true
                }
            }

            }
            } // grid

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
