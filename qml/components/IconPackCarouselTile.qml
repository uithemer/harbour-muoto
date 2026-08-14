import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0

Item {
    id: root

    property string packName: ""
    property string packDisplayName: ""

    width: parent ? parent.width : implicitWidth
    height: parent ? parent.height : implicitHeight

    function refreshPreview() {
        if (packName && packName !== "")
            iconapplier.buildPreview(packName)
    }

    onPackNameChanged: refreshPreview()
    Component.onCompleted: refreshPreview()

    Column {
        anchors.fill: parent
        spacing: Theme.paddingSmall

        Item {
            id: previewHost
            width: parent.width
            height: (parent.height - parent.spacing) * 0.58

            Item {
                width: Theme.iconSizeLarge
                height: Theme.iconSizeLarge
                anchors.centerIn: parent
                clip: true

                Image {
                    // Pack preview montage is 3×3; show the top-left tile only.
                    width: parent.width * 3
                    height: parent.height * 3
                    asynchronous: true
                    cache: false
                    fillMode: Image.PreserveAspectFit
                    source: packName !== ""
                            ? ("image://muoto/preview/" + packName + "?t=carousel")
                            : ""
                }
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
