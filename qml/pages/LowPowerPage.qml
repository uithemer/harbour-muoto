import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0
import "../components"

Dialog {
    id: dlg

    property bool pendingEnabled: false
    property bool appliedEnabled: false

    readonly property bool dirty: pendingEnabled !== appliedEnabled

    canAccept: MceLpm.available && dirty

    function initFromMce() {
        MceLpm.refresh()
        pendingEnabled = MceLpm.enabled
        appliedEnabled = MceLpm.enabled
    }

    Component.onCompleted: initFromMce()

    onStatusChanged: {
        if (status === PageStatus.Active)
            initFromMce()
    }

    onAccepted: {
        if (!MceLpm.applyProfile(pendingEnabled))
            app.showToast(qsTr("Could not update low-power mode settings"))
    }

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentHeight: content.height

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width

            DialogHeader {
                dialog: dlg
                cancelText: qsTr("Cancel")
                acceptText: qsTr("Apply")
            }

            Grid {
                width: parent.width
                columns: isLandscape ? 2 : 1

                Column {
                    width: isLandscape ? parent.width / 2 : parent.width

                    Item {
                        id: previewHost
                        width: parent.width
                        height: Math.min(parent.width, Math.max(280, flickable.height * 0.32))

                        LowPowerPreview {
                            anchors.fill: parent
                            enabled: dlg.pendingEnabled
                        }
                    }
                }

                Column {
                    width: isLandscape ? parent.width / 2 : parent.width

                    TextSwitch {
                        automaticCheck: false
                        text: qsTr("Low-power mode")
                        description: qsTr("Automatically show time and status information when taking the device out of pocket.")
                        checked: dlg.pendingEnabled
                        enabled: MceLpm.available
                        onClicked: dlg.pendingEnabled = !dlg.pendingEnabled
                    }

                    MuotoTextLabel {
                        visible: dlg.pendingEnabled
                        text: qsTr("Also wakes when you hover over the sensor, and keeps the proximity sensor ready so glance works reliably.")
                    }

                    MuotoTextLabel {
                        text: qsTr("Also known as Sneak Peek. Shows when the device is uncovered; double tap to wake fully.")
                    }

                    MuotoTextLabel {
                        visible: !MceLpm.available
                        text: qsTr("Low-power mode settings are not available on this device.")
                    }
                }
            }

            LabelSpacer { }
        }
    }
}
