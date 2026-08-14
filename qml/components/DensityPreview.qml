import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root

    property int iconPx: Theme.iconSizeLauncher
    property real fontScale: 1
    // Pixel ratio used to pick the silica z* Settings asset (mosaic: Theme;
    // density dialog: pending slider value).
    property real pixelRatio: Theme.pixelRatio

    function settingsIconUrlForRatio(pr) {
        var z = "z1.0"
        if (pr >= 2.5)
            z = "z2.5"
        else if (pr >= 2.0)
            z = "z2.0"
        else if (pr >= 1.75)
            z = "z1.75"
        else if (pr >= 1.5)
            z = "z1.5"
        else if (pr >= 1.25)
            z = "z1.25"
        return "file:///usr/share/themes/sailfish-default/silica/" + z
               + "/icons/icon-launcher-settings.png"
    }

    readonly property url settingsIconUrl: settingsIconUrlForRatio(pixelRatio)

    width: parent ? parent.width : implicitWidth
    height: parent ? parent.height : implicitHeight

    Column {
        id: col
        anchors.centerIn: parent
        spacing: Theme.paddingSmall
        width: parent.width

        Image {
            width: root.iconPx
            height: width
            anchors.horizontalCenter: parent.horizontalCenter
            asynchronous: true
            cache: true
            fillMode: Image.PreserveAspectFit
            source: root.settingsIconUrl
        }

        Label {
            id: caption
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Aa Bb")
            color: Theme.primaryColor
            font.pixelSize: Theme.fontSizeMedium * root.fontScale
        }
    }
}
