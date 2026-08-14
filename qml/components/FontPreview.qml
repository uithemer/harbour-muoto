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
    readonly property alias loadedFamily: previewfont.name

    FontLoader {
        id: previewfont
    }

    Component.onCompleted: {
        if (packName !== "" && selectedFont !== "")
            previewfont.source = FontWeightUtils.fontTtfPath(packName, selectedFont)
    }

    Label {
        id: previewlabel
        visible: compact
        anchors.fill: parent
        anchors.margins: Theme.paddingSmall
        font.family: previewfont.name
        font.weight: FontWeightUtils.fontWeightFromBasename(selectedFont)
        font.pixelSize: Theme.fontSizeSmall
        text: qsTr("Aa Bb Cc 123")
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        clip: true
    }

    Column {
        visible: !compact
        anchors.fill: parent
        anchors.margins: Theme.paddingLarge
        spacing: Theme.paddingMedium

        Label {
            width: parent.width
            font.family: previewfont.name
            font.weight: FontWeightUtils.fontWeightFromBasename(selectedFont)
            font.pixelSize: Theme.fontSizeExtraLarge
            text: "Lorem ipsum"
            wrapMode: Text.WordWrap
        }

        Label {
            width: parent.width
            font.family: previewfont.name
            font.weight: FontWeightUtils.fontWeightFromBasename(selectedFont)
            font.pixelSize: Theme.fontSizeMedium
            text: "Dolor sit amet, consectetur adipiscing elit. Maecenas imperdiet finibus venenatis. Suspendisse mollis urna sed luctus sodales."
            wrapMode: Text.WordWrap
        }
    }
}
