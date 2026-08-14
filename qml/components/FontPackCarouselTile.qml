import QtQuick 2.0
import Sailfish.Silica 1.0
import "../common/fontWeightUtils.js" as FontWeightUtils

Item {
    id: root

    property string packName: ""
    property string packDisplayName: ""
    property string sampleFontBasename: ""

    width: parent ? parent.width : implicitWidth
    height: parent ? parent.height : implicitHeight

    FontLoader {
        id: sampleFont
        source: sampleFontBasename !== ""
                ? FontWeightUtils.fontTtfPath(root.packName, sampleFontBasename)
                : ""
    }

    Column {
        anchors.fill: parent
        spacing: Theme.paddingSmall

        Item {
            id: previewHost
            width: parent.width
            height: (parent.height - parent.spacing) * 0.58

            Label {
                anchors.centerIn: parent
                text: "Aa"
                font.family: sampleFontBasename !== "" ? sampleFont.name : ""
                font.weight: FontWeightUtils.fontWeightFromBasename(sampleFontBasename)
                font.pixelSize: Theme.fontSizeLarge * 1.25
                color: sampleFontBasename !== "" ? Theme.primaryColor : Theme.secondaryColor
            }
        }

        Label {
            width: parent.width
            height: parent.height - previewHost.height - parent.spacing
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignTop
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryColor
            text: root.packDisplayName
        }
    }
}
