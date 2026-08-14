import QtQuick 2.0
import Sailfish.Silica 1.0
import "../common/fontWeightUtils.js" as FontWeightUtils

MouseArea {
    id: root

    property alias text: label.text
    property string packName: ""
    property string fontWeight: ""

    property bool checked
    property bool automaticCheck: true
    property real leftMargin
    property real rightMargin: Theme.paddingLarge
    property bool down: pressed && containsMouse
    property bool highlighted: down

    width: parent ? parent.width : Screen.width
    implicitHeight: Math.max(toggle.height, label.height)

    Item {
        id: toggle

        width: Theme.itemSizeExtraSmall
        height: Theme.itemSizeSmall
        anchors {
            left: parent.left
            leftMargin: root.leftMargin
        }

        GlassItem {
            id: indicator
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            opacity: root.enabled ? 1.0 : 0.4
            dimmed: !checked
            falloffRadius: checked ? defaultFalloffRadius : 0.075
            Behavior on falloffRadius {
                NumberAnimation { duration: 50; easing.type: Easing.InOutQuad }
            }
            brightness: 1.0
            color: highlighted ? Theme.highlightColor : Theme.primaryColor
        }
    }

    Label {
        id: label
        width: parent.width - toggle.width - root.leftMargin - root.rightMargin
        opacity: root.enabled ? 1.0 : 0.4
        anchors {
            verticalCenter: toggle.verticalCenter
            verticalCenterOffset: lineCount > 1 ? (lineCount - 1) * height / lineCount / 2 : 0
            left: toggle.right
        }
        wrapMode: Text.Wrap
        color: highlighted ? Theme.highlightColor : Theme.primaryColor
        font.weight: FontWeightUtils.fontWeightFromBasename(fontWeight)
    }

    onClicked: {
        if (automaticCheck)
            checked = !checked
    }
}
