import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    anchors.fill: parent

    property string packName: ""
    property string selectedFont: ""

    readonly property string previewSource: {
        var c = String(Theme.primaryColor).replace("#", "")
        var pack = (!packName || packName === "") ? "default" : packName
        if (pack === "default")
            return "image://muoto-font/lorem/default?c=" + c
        if (selectedFont === "")
            return ""
        return "image://muoto-font/lorem/" + pack + "/" + selectedFont + "?c=" + c
    }

    Image {
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
            topMargin: Theme.paddingLarge
        }
        height: parent.height - Theme.paddingLarge * 2
        fillMode: Image.PreserveAspectFit
        asynchronous: false
        sourceSize.width: Math.max(1, width)
        visible: previewSource !== ""
        source: previewSource
    }
}
