import QtQuick 2.0
import Sailfish.Silica 1.0

BackgroundItem {
    id: root

    property string title: ""
    property string subtitle: ""
    property int previewHeight: Theme.itemSizeLarge * 1.5
    default property alias previewContent: previewHost.data

    width: parent ? parent.width : implicitWidth
    implicitHeight: previewHeight + labelColumn.height + Theme.paddingLarge * 2
    height: implicitHeight
    clip: true

    Column {
        id: labelColumn

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.paddingMedium
        spacing: Theme.paddingSmall

        Label {
            width: parent.width
            text: root.title
            font.pixelSize: Theme.fontSizeMedium
            color: root.highlighted || root.down ? Theme.highlightColor : Theme.primaryColor
            truncationMode: TruncationMode.Fade
        }

        Label {
            width: parent.width
            text: root.subtitle
            font.pixelSize: Theme.fontSizeSmall
            color: root.highlighted || root.down ? Theme.secondaryHighlightColor : Theme.secondaryColor
            truncationMode: TruncationMode.Fade
        }

    }

    Item {
        id: previewHost

        height: root.previewHeight
        clip: true

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: Theme.paddingMedium
        }

    }

}
