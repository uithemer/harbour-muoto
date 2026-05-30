import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    property alias icon: coverIcon.source
    property alias label: label.text

    width: parent.width
    height: label.height + Theme.paddingSmall

    HighlightImage {
        id: coverIcon
        highlighted: true
        anchors {
            left: parent.left
            leftMargin: Theme.paddingMedium
            rightMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmall
        height: Theme.iconSizeSmall
    }

    Label {
        id: label
        anchors {
            right: parent.right
            left: coverIcon.right
            leftMargin: Theme.paddingMedium
            rightMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }
        x: Theme.paddingSmall
        width: parent.width - (x * 2)
        font.pixelSize: Theme.fontSizeSmall
        color: Theme.highlightColor
        wrapMode: Text.Wrap
        lineHeight: 1
        maximumLineCount: 2
    }

}
