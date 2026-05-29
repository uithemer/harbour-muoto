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
            topMargin: Theme.paddingLarge
        }
        font.family: previewfont.name
        font.weight: FontWeightUtils.fontWeightFromBasename(selectedFont)
        font.pixelSize: Theme.fontSizeExtraSmall
        text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas imperdiet finibus venenatis."
        wrapMode: Text.WordWrap
    }

}
