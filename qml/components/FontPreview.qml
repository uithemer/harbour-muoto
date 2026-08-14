import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root
    clip: true

    property string packName: ""
    property string selectedFont: ""

    readonly property int previewHeadingPx: {
        // Leave room for heading + gap + several wrapped body lines.
        var cap = Math.max(1, Math.round(height * 0.14))
        return Math.min(Theme.fontSizeLarge, cap)
    }
    readonly property int previewBodyPx: {
        var cap = Math.max(1, Math.round(height * 0.09))
        return Math.min(Theme.fontSizeSmall, cap)
    }

    readonly property string previewSource: {
        var c = String(Theme.primaryColor).replace("#", "")
        var pack = (!packName || packName === "") ? "default" : packName
        var q = "?c=" + c + "&h=" + previewHeadingPx + "&b=" + previewBodyPx
        if (pack === "default")
            return "image://muoto-font/lorem/default" + q
        if (selectedFont === "")
            return ""
        return "image://muoto-font/lorem/" + pack + "/" + selectedFont + q
    }

    Image {
        id: previewImage
        anchors.fill: parent
        anchors.margins: Theme.paddingMedium
        fillMode: Image.PreserveAspectFit
        asynchronous: false
        sourceSize.width: Math.max(1, width)
        sourceSize.height: Math.max(1, height)
        visible: source !== ""
        source: (width > 8 && height > 8 && root.previewSource !== "")
                ? root.previewSource : ""
    }
}
