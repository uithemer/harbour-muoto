import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root
    clip: true

    property string packName: ""
    property string selectedFont: ""
    property bool compact: false

    readonly property string previewSource: {
        var c = String(Theme.primaryColor).replace("#", "")
        var kind = compact ? "aa" : "lorem"
        var pack = (!packName || packName === "") ? "default" : packName
        var q = "?c=" + c
        if (!compact)
            q += "&h=" + Theme.fontSizeLarge + "&b=" + Theme.fontSizeSmall
        if (pack === "default")
            return "image://muoto-font/" + kind + "/default" + q
        if (selectedFont === "")
            return ""
        return "image://muoto-font/" + kind + "/" + pack + "/" + selectedFont + q
    }

    Image {
        id: previewImage
        anchors.fill: parent
        fillMode: compact ? Image.PreserveAspectFit : Image.Pad
        asynchronous: false
        sourceSize.width: Math.max(1, width)
        sourceSize.height: Math.max(1, height)
        visible: source !== ""
        source: (width > 8 && height > 8 && root.previewSource !== "")
                ? root.previewSource : ""
    }
}
