import Qt.labs.folderlistmodel 2.1
import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root

    property string packName: ""
    property string packDisplayName: ""
    property bool hasNative: false
    property bool hasApk: false
    property bool hasJolla: false
    property bool isDefault: false
    property bool highlighted: false
    property var stockModel: null
    property url lockedThumb: ""
    readonly property string thumbFolder: {
        if (isDefault || !packName || packName === "")
            return "";

        var rootPath = "file:///usr/share/" + packName;
        if (hasNative)
            return rootPath + "/native/86x86/apps";

        if (hasApk)
            return rootPath + "/apk/86x86";

        if (hasJolla)
            return rootPath + "/jolla/z1.0/icons";

        return "";
    }

    function applyPick() {
        var url = "";
        if (root.isDefault) {
            var m = root.stockModel;
            if (m && m.count > 0) {
                // FolderAmbient themes icon-launcher-folder-* inplace; never
                // use those for the Default stock thumb.
                var tries = Math.min(m.count, 32);
                for (var t = 0; t < tries; ++t) {
                    var candidate = m.get(Math.floor(Math.random() * m.count), "fileURL");
                    var name = String(candidate).split("/").pop();
                    if (name.indexOf("icon-launcher-folder-") !== 0) {
                        url = candidate;
                        break;
                    }
                }
            }
        } else {
            var n = thumbs.count;
            if (n > 0)
                url = thumbs.get(Math.floor(Math.random() * n), "fileURL");

        }
        if (lockedThumb !== url)
            lockedThumb = url;

    }

    width: parent ? parent.width : implicitWidth
    height: parent ? parent.height : implicitHeight
    onIsDefaultChanged: {
        lockedThumb = "";
        pickTimer.restart();
    }
    onThumbFolderChanged: {
        lockedThumb = "";
        pickTimer.restart();
    }
    Component.onCompleted: pickTimer.restart()

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
                pickTimer.restart();

        }
    }

    Timer {
        id: pickTimer

        interval: 50
        onTriggered: applyPick()
    }

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
            color: root.highlighted ? Theme.highlightColor : Theme.secondaryColor
            text: root.packDisplayName
        }

    }

}
