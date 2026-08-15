import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root

    property string packName: ""
    property int previewHeight: Math.round((parent ? parent.width : width) / 2)
    property int previewMargins: Theme.paddingMedium
    property string _previewBuiltPack: ""
    property string _previewLoadedPack: ""
    property bool _refreshDeferred: false
    readonly property bool previewAvailable: _previewLoadedPack !== "" && _previewLoadedPack === packName

    function refresh() {
        // buildPreview samples and montages on the QML thread. An apply
        // rewrites activeIconPack mid-flight, so without this every tile
        // would re-render while the daemon is still working.
        if (settings.isRunning) {
            _refreshDeferred = true;
            return ;
        }
        _refreshDeferred = false;
        if (!packName || packName === "") {
            busyimg.running = false;
            imgpreview.source = "";
            imgpreviewfallback.visible = true;
            return ;
        }
        if (packName === _previewBuiltPack && previewAvailable)
            return ;

        _previewBuiltPack = packName;
        _previewLoadedPack = "";
        busyimg.running = true;
        imgpreviewfallback.visible = false;
        imgpreview.source = "";
        iconapplier.buildPreview(packName);
    }

    width: parent ? parent.width : implicitWidth
    height: previewHeight
    onPackNameChanged: refresh()
    Component.onCompleted: refresh()

    Connections {
        target: settings
        onIsRunningChanged: {
            if (!settings.isRunning && root._refreshDeferred)
                root.refresh();

        }
    }

    Connections {
        target: iconapplier
        onPreviewReady: {
            if (packName !== root.packName)
                return ;

            busyimg.running = false;
            if (ok) {
                if (_previewLoadedPack === root.packName)
                    return ;

                _previewLoadedPack = root.packName;
                imgpreviewfallback.visible = false;
                imgpreview.source = "image://muoto/preview/" + root.packName + "?t=" + Date.now();
            } else {
                _previewLoadedPack = "";
                imgpreview.source = "";
                imgpreviewfallback.visible = true;
            }
        }
    }

    BusyIndicator {
        id: busyimg

        running: false
        size: BusyIndicatorSize.Medium
        anchors.centerIn: parent
    }

    Image {
        id: imgpreview

        anchors.fill: parent
        anchors.margins: root.previewMargins
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        cache: false
        visible: status === Image.Ready
        onStatusChanged: {
            if (status === Image.Ready || status === Image.Error)
                busyimg.running = false;

        }
    }

    Label {
        id: imgpreviewfallback

        visible: false
        anchors.centerIn: parent
        width: parent.width - Theme.paddingLarge * 2
        horizontalAlignment: Text.AlignHCenter
        color: Theme.secondaryColor
        text: qsTr("No preview available")
    }

}
