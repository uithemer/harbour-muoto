import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root

    property int iconPx: Theme.iconSizeLauncher
    property real fontScale: 1

    width: parent ? parent.width : implicitWidth
    height: parent ? parent.height : implicitHeight

    Column {
        id: col

        anchors.centerIn: parent
        spacing: Theme.paddingSmall
        width: parent.width

        Image {
            width: root.iconPx
            height: width
            anchors.horizontalCenter: parent.horizontalCenter
            asynchronous: true
            cache: true
            fillMode: Image.PreserveAspectFit
            source: "image://theme/icon-launcher-settings"
            sourceSize.width: width
            sourceSize.height: height
        }

        Label {
            id: caption

            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: "Aa Bb"
            color: Theme.primaryColor
            font.pixelSize: Theme.fontSizeMedium * root.fontScale
        }

    }

}
