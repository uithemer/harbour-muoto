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
        if (pack === "default")
            return "image://muoto-font/" + kind + "/default?c=" + c
        if (selectedFont === "")
            return ""
        return "image://muoto-font/" + kind + "/" + pack + "/" + selectedFont
               + "?c=" + c
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
