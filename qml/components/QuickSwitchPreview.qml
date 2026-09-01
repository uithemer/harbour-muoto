import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root

    property bool enabled: false
    // Visible crop height — the phone body is taller than the tile shows, so
    // everything worth seeing is anchored to the top of the screen.
    readonly property real cropHeight: height > 0 ? height : Theme.itemSizeLarge * 1.5

    width: parent ? parent.width : implicitWidth
    height: parent ? parent.height : implicitHeight
    clip: true

    Rectangle {
        id: phone

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: Theme.paddingSmall
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

            // The previous app's cover, revealed at the edge by the peek. It
            // only highlights once the gesture threshold is passed, so keep it
            // dim while quick switching is off.
            Rectangle {
                id: previousApp

                anchors.left: parent.left
                anchors.leftMargin: Theme.paddingSmall / 2
                anchors.top: parent.top
                anchors.topMargin: Theme.paddingLarge
                width: parent.width * 0.36
                height: root.cropHeight * 0.4
                radius: Theme.paddingSmall / 2
                color: Theme.rgba(Theme.highlightBackgroundColor, 0.45)
                border.width: 1
                border.color: Theme.rgba(Theme.highlightColor, 0.7)

                Image {
                    anchors.centerIn: parent
                    width: Math.min(Theme.iconSizeSmall, parent.width * 0.6)
                    height: width
                    sourceSize.width: width
                    sourceSize.height: height
                    fillMode: Image.PreserveAspectFit
                    source: "image://theme/icon-m-right?" + Theme.highlightColor
                }

            }

            // Foreground app, pushed aside by the peek gesture. Opaque: with
            // the gesture off it has to hide the cover behind it completely.
            Rectangle {
                id: foregroundApp

                anchors.top: parent.top
                anchors.bottom: parent.bottom
                x: root.enabled ? parent.width * 0.46 : 0
                width: parent.width
                radius: Theme.paddingSmall
                color: Qt.tint("black", Theme.rgba(Theme.primaryColor, 0.16))
                border.width: 1
                border.color: Theme.rgba(Theme.primaryColor, 0.3)

                Column {
                    spacing: Theme.paddingSmall

                    anchors {
                        top: parent.top
                        left: parent.left
                        right: parent.right
                        topMargin: Theme.paddingLarge
                        leftMargin: Theme.paddingMedium
                        rightMargin: Theme.paddingMedium
                    }

                    Repeater {
                        model: 4

                        Rectangle {
                            width: parent.width * (index === 3 ? 0.5 : 1)
                            height: Math.max(2, Math.round(root.cropHeight * 0.035))
                            radius: height / 2
                            color: Theme.rgba(Theme.primaryColor, index === 0 ? 0.55 : 0.28)
                        }

                    }

                }

                Behavior on x {
                    NumberAnimation {
                        duration: 200
                        easing.type: Easing.InOutQuad
                    }

                }

            }

        }

    }

}
