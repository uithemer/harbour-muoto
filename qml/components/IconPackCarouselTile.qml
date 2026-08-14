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

    width: parent ? parent.width : implicitWidth
    height: parent ? parent.height : implicitHeight

    readonly property string thumbFolder: {
        if (!packName || packName === "")
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

    FolderListModel {
        id: thumbs
        folder: root.thumbFolder
        nameFilters: ["*.png"]
        showDirs: false
        showFiles: true
        showHidden: false
        showOnlyReadable: true
        sortField: FolderListModel.Unsorted
    }

    Column {
        anchors.fill: parent
        spacing: Theme.paddingSmall

        Item {
            id: previewHost
            width: parent.width
            height: (parent.height - parent.spacing) * 0.58

            Image {
                width: Theme.iconSizeLarge
                height: Theme.iconSizeLarge
                anchors.centerIn: parent
                asynchronous: true
                cache: false
                fillMode: Image.PreserveAspectFit
                source: thumbs.count > 0 ? thumbs.get(0, "fileURL") : ""
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
