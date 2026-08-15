import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root

    property bool enabled: false
    property int clockTick: 0
    // Visible crop height — fonts scale against this, not the full phone body.
    readonly property real cropHeight: height > 0 ? height : Theme.itemSizeLarge * 1.5

    width: parent ? parent.width : implicitWidth
    height: parent ? parent.height : implicitHeight
    clip: true

    Rectangle {
        id: phone

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: Theme.paddingSmall
        // Wide enough to read; tall full-body silhouette mostly clipped away.
        width: Math.min(parent.width * 0.72, Math.max(Theme.itemSizeLarge, parent.height * 1.15))
        height: width * 1.7
        radius: Theme.paddingMedium
        color: Theme.rgba(Theme.highlightBackgroundColor, 0.12)
        border.width: Math.max(1, Math.round(Theme.paddingSmall * 0.35))
        border.color: Theme.rgba(Theme.primaryColor, 0.25)

        Rectangle {
            id: screen

            radius: Theme.paddingSmall
            color: "black"
            clip: true

            anchors {
                fill: parent
                margins: Theme.paddingSmall
            }

            Column {
                spacing: Theme.paddingSmall
                opacity: root.enabled ? 1 : 0

                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    topMargin: Theme.paddingLarge * 1.6
                    leftMargin: Theme.paddingSmall
                    rightMargin: Theme.paddingSmall
                }

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: {
                        var _ = root.clockTick;
                        return Qt.formatTime(new Date(), "hh:mm");
                    }
                    color: Theme.highlightColor
                    opacity: 0.85
                    font.pixelSize: Math.max(Theme.fontSizeLarge, Math.round(root.cropHeight * 0.28))
                    font.family: Theme.fontFamilyHeading
                }

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: {
                        var _ = root.clockTick;
                        return Qt.formatDate(new Date(), Locale.ShortFormat);
                    }
                    color: Theme.highlightColor
                    opacity: 0.55
                    font.pixelSize: Math.max(Theme.fontSizeExtraSmall, Math.round(root.cropHeight * 0.1))
                    truncationMode: TruncationMode.Fade
                }

                Behavior on opacity {
                    FadeAnimation {
                    }

                }

            }

        }

    }

    Timer {
        interval: 30 * 1000
        running: root.enabled && root.visible && Qt.application.active
        repeat: true
        onTriggered: root.clockTick++
    }

}
