import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root
    clip: true

    property string packName: ""
    property string selectedFont: ""

    readonly property string previewSource: {
        var c = String(Theme.primaryColor).replace("#", "")
        var pack = (!packName || packName === "") ? "default" : packName
        var q = "?c=" + c + "&h=" + Theme.fontSizeLarge + "&b=" + Theme.fontSizeSmall
        if (pack === "default")
            return "image://muoto-font/lorem/default" + q
        if (selectedFont === "")
            return ""
        return "image://muoto-font/lorem/" + pack + "/" + selectedFont + q
    }

    Image {
        id: previewImage
        anchors.fill: parent
        fillMode: Image.Pad
        asynchronous: false
        sourceSize.width: Math.max(1, width)
        sourceSize.height: Math.max(1, height)
        visible: source !== ""
        source: (width > 8 && height > 8 && root.previewSource !== "")
                ? root.previewSource : ""
    }
}
