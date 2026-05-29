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
    canAccept: itsrestoredpr.checked || itsrestoreis.checked

    BusyState { id: busyindicator }

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

            HomescreenRestartSection {
                settings: dlgrestore.settings
                explanation: qsTr("Restart the homescreen after restoring display density so all apps pick up the changes.")
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }
        }
    }
}
