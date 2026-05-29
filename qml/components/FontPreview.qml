import QtQuick 2.0
import Sailfish.Silica 1.0
import "../common/fontWeightUtils.js" as FontWeightUtils

Item {
    anchors.fill: parent

    property string packName: ""
    property string selectedFont: ""

    FontLoader {
        id: previewfont
        source: FontWeightUtils.fontTtfPath(packName, selectedFont)
    }

    Label {
        id: previewlabel
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
        }
        height: 350
        font.family: previewfont.name
        font.weight: FontWeightUtils.fontWeightFromBasename(selectedFont)
        text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas imperdiet finibus venenatis. Suspendisse mollis urna sed luctus sodales."
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
    }

}
