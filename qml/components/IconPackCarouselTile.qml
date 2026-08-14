import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0

Item {
    id: root

    property string packName: ""
    property string packDisplayName: ""

    width: parent ? parent.width : implicitWidth
    height: parent ? parent.height : implicitHeight

    Column {
        anchors.fill: parent
        spacing: Theme.paddingSmall

        Item {
            width: parent.width
            height: width

            IconPackPreview {
                anchors.fill: parent
                packName: root.packName
                previewHeight: height
            }
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryColor
            truncationMode: TruncationMode.Fade
            text: root.packDisplayName
        }
    }
}
