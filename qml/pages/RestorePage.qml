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
    canAccept: itsrestoreicons.checked || itsrestorefonts.checked

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
                text: "<br>" + qsTr("Launcher icons refresh automatically when you restore. Enable below only if icons stay stale (full lipstick restart).")
            }

            TextSwitch {
                text: qsTr("Restart homescreen (fallback)")
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
