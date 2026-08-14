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
        anchors.fill: parent
        anchors.margins: compact ? Theme.paddingSmall : Theme.paddingLarge
        fillMode: Image.PreserveAspectFit
        asynchronous: false
        sourceSize.width: Math.max(1, width)
        sourceSize.height: Math.max(1, compact ? height : width / 2)
        visible: root.previewSource !== ""
        source: root.previewSource
    }
}
