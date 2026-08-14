import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root

    property string packName: ""
    property string packDisplayName: ""
    property string sampleFontBasename: ""
    property bool isDefault: false

    width: parent ? parent.width : implicitWidth
    height: parent ? parent.height : implicitHeight

    readonly property int aaPx: Math.round(Theme.fontSizeLarge * 2)

    readonly property string aaSource: {
        var c = String(Theme.primaryColor).replace("#", "")
        if (root.isDefault)
            return "image://muoto-font/aa/default?c=" + c
        if (root.packName === "" || root.sampleFontBasename === "")
            return ""
        return "image://muoto-font/aa/" + root.packName + "/"
               + root.sampleFontBasename + "?c=" + c
    }

    Column {
        anchors.fill: parent
        spacing: Theme.paddingSmall

        Item {
            id: previewHost
            width: parent.width
            height: (parent.height - parent.spacing) * 0.58

            Image {
                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height)
                height: width
                fillMode: Image.PreserveAspectFit
                asynchronous: false
                sourceSize.width: root.aaPx
                sourceSize.height: root.aaPx
                visible: source !== ""
                source: (previewHost.width > 8 && previewHost.height > 8
                         && root.aaSource !== "") ? root.aaSource : ""
            }

            Label {
                anchors.centerIn: parent
                visible: root.aaSource === ""
                text: "Aa"
                font.pixelSize: Theme.fontSizeLarge * 1.25
                color: Theme.primaryColor
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
