import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0
import "../components"

Dialog {
    id: dlg

    property bool pendingEnabled: false
    property bool pendingFromPocket: true
    property bool pendingHover: false
    property bool pendingProximityReady: false

    property bool appliedEnabled: false
    property bool appliedFromPocket: true
    property bool appliedHover: false
    property bool appliedProximityReady: false

    readonly property bool dirty: pendingEnabled !== appliedEnabled
                                  || pendingFromPocket !== appliedFromPocket
                                  || pendingHover !== appliedHover
                                  || pendingProximityReady !== appliedProximityReady

    canAccept: MceLpm.available && dirty

    function initFromMce() {
        MceLpm.refresh()
        pendingEnabled = MceLpm.enabled
        pendingFromPocket = MceLpm.triggerFromPocket
        pendingHover = MceLpm.triggerFromPocket && MceLpm.triggerHoverOver
        pendingProximityReady = MceLpm.proximityReady
        appliedEnabled = pendingEnabled
        appliedFromPocket = pendingFromPocket
        appliedHover = pendingHover
        appliedProximityReady = pendingProximityReady
    }

    function setRecommended() {
        pendingEnabled = true
        pendingFromPocket = true
        pendingHover = true
        pendingProximityReady = true
    }

    function setDefaults() {
        pendingEnabled = false
        pendingFromPocket = true
        pendingHover = false
        pendingProximityReady = false
    }

    onPendingFromPocketChanged: {
        if (!pendingFromPocket)
            pendingHover = false
    }

    Component.onCompleted: initFromMce()

    onStatusChanged: {
        if (status === PageStatus.Active)
            initFromMce()
    }

    onAccepted: {
        var hover = pendingFromPocket && pendingHover
        if (MceLpm.apply(pendingEnabled, pendingFromPocket, hover,
                         pendingProximityReady))
            app.showToast(qsTr("Settings applied."))
        else
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

            LabelSpacer { }
                }

                Column {
                    width: isLandscape ? parent.width / 2 : parent.width

                    TextSwitch {
                        automaticCheck: false
                        text: qsTr("Low-power mode")
                        description: qsTr("Automatically show time and status information when the screen is off.")
                        checked: dlg.pendingEnabled
                        enabled: MceLpm.available
                        onClicked: dlg.pendingEnabled = !dlg.pendingEnabled
                    }

                    TextSwitch {
                        automaticCheck: false
                        text: qsTr("Wake from pocket")
                        description: qsTr("Show the glance screen when taking the device out of pocket.")
                        checked: dlg.pendingFromPocket
                        enabled: MceLpm.available
                        onClicked: dlg.pendingFromPocket = !dlg.pendingFromPocket
                    }

                    TextSwitch {
                        automaticCheck: false
                        text: qsTr("Wake on hover")
                        description: qsTr("Show the glance screen when you hold your hand over the sensor.")
                        checked: dlg.pendingHover
                        enabled: MceLpm.available && dlg.pendingFromPocket
                        onClicked: dlg.pendingHover = !dlg.pendingHover
                    }

                    TextSwitch {
                        automaticCheck: false
                        text: qsTr("Keep proximity ready")
                        description: qsTr("Needed for reliable glance on many devices. Enabling this may use more battery, and the screen may not turn off reliably during calls.")
                        checked: dlg.pendingProximityReady
                        enabled: MceLpm.available
                        onClicked: dlg.pendingProximityReady = !dlg.pendingProximityReady
                    }

            LabelSpacer { }
                    MuotoTextLabel {
                        text: qsTr("Double tap the glance screen to wake fully.")
                    }

            LabelSpacer { }
                    ButtonLayout {
                        preferredWidth: Theme.buttonWidthMedium
                        Button {
                            text: qsTr("Use recommended")
                            enabled: MceLpm.available
                            onClicked: dlg.setRecommended()
                        }
                        Button {
                            text: qsTr("Restore defaults")
                            enabled: MceLpm.available
                            onClicked: dlg.setDefaults()
                        }
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
