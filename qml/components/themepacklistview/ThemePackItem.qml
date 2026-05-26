import QtQuick 2.0
import Sailfish.Silica 1.0

ListItem
{
    property bool iconInstalled: false
    property bool fontInstalled: false

    signal uninstallRequested()

    id: themepackitem
    width: parent.width
    contentHeight: Theme.itemSizeSmall

//    Rectangle { id: rect; anchors.fill: parent; visible: fontInstalled || iconInstalled; color: Theme.rgba(Theme.highlightBackgroundColor, 0.5) }

    Label {
        anchors {
            fill: parent
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
        }
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter

        textFormat: Text.RichText

        text: {
            var s = model.packDisplayName;
            var types = [];

            if(iconInstalled)
                types.push(qsTr("icons"));

            if(fontInstalled)
                types.push(qsTr("fonts"));

            if(types.length <= 0)
                return s;

            return s + "&nbsp;<font style='color:" + Theme.highlightColor  + ";background-color:" + Theme.rgba(Theme.highlightBackgroundColor, 0.5) +"'>&nbsp;" + types.join("&nbsp;</font>&nbsp;<font style='color:" + Theme.highlightColor  + ";background-color:" + Theme.rgba(Theme.highlightBackgroundColor, 0.5) +"'>&nbsp;") + "&nbsp;</font>";
        }
    }

    menu: ContextMenu {
        MenuItem { text: qsTr("Uninstall"); onClicked: uninstallRequested() }
    }
}
