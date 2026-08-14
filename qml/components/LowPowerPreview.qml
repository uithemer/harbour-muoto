import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root

    property bool enabled: false
    property int clockTick: 0

    width: parent ? parent.width : implicitWidth
    height: parent ? parent.height : implicitHeight

    Rectangle {
        id: phone
        anchors.centerIn: parent
        width: Math.min(parent.width * 0.55, parent.height * 0.62)
        height: width * 1.7
        radius: Theme.paddingMedium
        color: Theme.rgba(Theme.highlightBackgroundColor, 0.12)
        border.width: Math.max(1, Math.round(Theme.paddingSmall * 0.35))
        border.color: Theme.rgba(Theme.primaryColor, 0.25)

        Rectangle {
            id: screen
            anchors {
                fill: parent
                margins: Theme.paddingSmall
            }
            radius: Theme.paddingSmall
            color: "black"

            Column {
                anchors.centerIn: parent
                width: parent.width - Theme.paddingMedium
                spacing: Theme.paddingSmall
                opacity: root.enabled ? 1.0 : 0.0
                Behavior on opacity { FadeAnimation { } }

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    // clockTick forces re-eval when the minute timer fires
                    text: {
                        var _ = root.clockTick
                        return Qt.formatTime(new Date(), "hh:mm")
                    }
                    color: Theme.highlightColor
                    opacity: 0.85
                    font.pixelSize: Math.max(Theme.fontSizeMedium,
                                             Math.round(screen.height * 0.18))
                    font.family: Theme.fontFamilyHeading
                }

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: {
                        var _ = root.clockTick
                        return Qt.formatDate(new Date(), Locale.ShortFormat)
                    }
                    color: Theme.highlightColor
                    opacity: 0.55
                    font.pixelSize: Math.max(Theme.fontSizeTiny,
                                             Math.round(screen.height * 0.07))
                    truncationMode: TruncationMode.Fade
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
