import QtQuick 2.0
import Sailfish.Silica 1.0
import "../common/fontWeightUtils.js" as FontWeightUtils

Item {
    id: root
    clip: true

    property string packName: ""
    property string selectedFont: ""
    property bool compact: false

    // Set source once. Changing FontLoader.source (or destroying it while
    // another loader still holds the same file) aborts in Qt 5.6 fontconfig.
    FontLoader {
        id: previewfont
    }

    Component.onCompleted: {
        if (packName !== "" && selectedFont !== "")
            previewfont.source = FontWeightUtils.fontTtfPath(packName, selectedFont)
    }

    Label {
        id: previewlabel
        anchors.fill: parent
        anchors.margins: compact ? Theme.paddingSmall : Theme.paddingLarge
        font.family: previewfont.name
        font.weight: FontWeightUtils.fontWeightFromBasename(selectedFont)
        font.pixelSize: compact ? Theme.fontSizeSmall : Theme.fontSizeMedium
        text: compact
              ? qsTr("Aa Bb Cc 123")
              : "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas imperdiet finibus venenatis. Suspendisse mollis urna sed luctus sodales."
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        clip: true
    }
}
