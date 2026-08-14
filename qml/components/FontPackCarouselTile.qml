import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0
import "../common/fontWeightUtils.js" as FontWeightUtils

Item {
    id: root

    property string packName: ""
    property string packDisplayName: ""

    width: parent ? parent.width : implicitWidth
    height: parent ? parent.height : implicitHeight

    FontWeightModel {
        id: weights
        packName: root.packName
    }

    readonly property string sampleBasename: weights.rowCount() > 0
        ? FontWeightUtils.fontBasenameFromFilename(weights.firstWeight)
        : ""

    FontLoader {
        id: sampleFont
        source: sampleBasename !== ""
                ? FontWeightUtils.fontTtfPath(root.packName, sampleBasename)
                : ""
    }

    Column {
        anchors.fill: parent
        spacing: Theme.paddingSmall

        Item {
            width: parent.width
            height: width

            Label {
                anchors.centerIn: parent
                text: "Aa"
                font.family: sampleBasename !== "" ? sampleFont.name : Theme.defaultFontFamily
                font.weight: FontWeightUtils.fontWeightFromBasename(sampleBasename)
                font.pixelSize: Theme.fontSizeLarge * 1.25
                color: sampleBasename !== "" ? Theme.primaryColor : Theme.secondaryColor
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
