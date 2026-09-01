import "../components"
import Nemo.Configuration 1.0
import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: dlg

    property bool pendingEnabled: false
    property bool appliedEnabled: false
    readonly property bool dirty: pendingEnabled !== appliedEnabled

    function initFromDconf() {
        quickSwitchKey.sync();
        appliedEnabled = quickSwitchKey.value === true;
        pendingEnabled = appliedEnabled;
    }

    canAccept: dirty
    Component.onCompleted: initFromDconf()
    onStatusChanged: {
        if (status === PageStatus.Active)
            initFromDconf();

    }
    onAccepted: {
        quickSwitchKey.value = pendingEnabled;
        quickSwitchKey.sync();
        appliedEnabled = pendingEnabled;
        app.showToast(pendingEnabled ? qsTr("Quick app switching enabled.") : qsTr("Quick app switching disabled."));
    }

    ConfigurationValue {
        id: quickSwitchKey

        key: "/desktop/sailfish/experimental/quickAppToggleGesture"
        defaultValue: false
    }

    SilicaFlickable {
        id: flickable

        anchors.fill: parent
        contentHeight: content.height

        VerticalScrollDecorator {
        }

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

                        QuickSwitchPreview {
                            anchors.fill: parent
                            enabled: dlg.pendingEnabled
                        }

                    }

                    LabelSpacer {
                    }

                }

                Column {
                    width: isLandscape ? parent.width / 2 : parent.width

                    TextSwitch {
                        automaticCheck: false
                        text: qsTr("Quick app switching")
                        description: qsTr("Jump straight back to the previous app, like Alt+Tab on a computer.")
                        checked: dlg.pendingEnabled
                        onClicked: dlg.pendingEnabled = !dlg.pendingEnabled
                    }

                    LabelSpacer {
                    }

                    MuotoTextLabel {
                        text: qsTr("Peek slowly from the edge of the screen, about three centimetres and for more than half a second. The previous app's cover highlights on the switcher; lift your finger to switch to it, or peek back towards the edge to cancel.")
                    }

                    MuotoTextLabel {
                        text: qsTr("This is an experimental Sailfish OS setting. It may behave differently or disappear in a future OS release.")
                    }

                    LabelSpacer {
                    }

                }

            }

            LabelSpacer {
            }

        }

    }

}
