import QtQuick 2.0
import Qt.labs.folderlistmodel 2.1
import Sailfish.Silica 1.0

Item {
    id: root

    property string packName: ""
    property string packDisplayName: ""
    property bool hasNative: false
    property bool hasApk: false
    property bool hasJolla: false
    property bool isDefault: false
    property var stockModel: null

    width: parent ? parent.width : implicitWidth
    height: parent ? parent.height : implicitHeight

    property url lockedThumb: ""

    readonly property string thumbFolder: {
        if (isDefault || !packName || packName === "")
            return ""
        var rootPath = "file:///usr/share/" + packName
        if (hasNative)
            return rootPath + "/native/86x86/apps"
        if (hasApk)
            return rootPath + "/apk/86x86"
        if (hasJolla)
            return rootPath + "/jolla/z1.0/icons"
        return ""
    }

    function applyPick() {
        var url = ""
        if (root.isDefault) {
            var m = root.stockModel
            if (m && m.count > 0)
                url = m.get(Math.floor(Math.random() * m.count), "fileURL")
        } else {
            var n = thumbs.count
            if (n > 0)
                url = thumbs.get(Math.floor(Math.random() * n), "fileURL")
        }
        if (lockedThumb !== url)
            lockedThumb = url
    }

    FolderListModel {
        id: thumbs
        folder: root.thumbFolder
        nameFilters: ["*.png"]
        showDirs: false
        showFiles: true
        showHidden: false
        showOnlyReadable: true
        sortField: FolderListModel.Unsorted
        onCountChanged: pickTimer.restart()
    }

    Connections {
        target: root.stockModel
        onCountChanged: {
            if (root.isDefault)
                pickTimer.restart()
        }
    }

    Timer {
        id: pickTimer
        interval: 50
        onTriggered: applyPick()
    }

    onIsDefaultChanged: {
        lockedThumb = ""
        pickTimer.restart()
    }
    onThumbFolderChanged: {
        lockedThumb = ""
        pickTimer.restart()
    }

    Component.onCompleted: pickTimer.restart()

    Column {
        anchors.fill: parent
        spacing: Theme.paddingSmall

        Item {
            id: previewHost
            width: parent.width
            height: (parent.height - parent.spacing) * 0.58

            Image {
                width: (Theme.iconSizeMedium + Theme.iconSizeLarge) / 2
                height: width
                anchors.centerIn: parent
                asynchronous: true
                cache: true
                fillMode: Image.PreserveAspectFit
                source: root.lockedThumb
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
