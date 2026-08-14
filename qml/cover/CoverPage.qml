import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

CoverBackground {
    Image {
        id: coverimg
        fillMode: Image.PreserveAspectFit
        source: app.isLightTheme ? "../../images/coverbg-light.png"
                                 : "../../images/coverbg-dark.png"
        opacity: {
            if (settings.isRunning)
                0.1
            else
                (settings.hasActiveIconPack() || settings.hasActiveFontPack()) ? 0.1 : 0.3
        }
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width
        height: sourceSize.height * width / sourceSize.width
    }

    Column {
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            leftMargin: Theme.paddingSmall
            rightMargin: Theme.paddingSmall
            topMargin: Theme.paddingLarge
        }
        spacing: Theme.paddingSmall
        visible: !settings.isRunning
        opacity: 0.8

        IconPackPreview {
            width: parent.width
            packName: settings.hasActiveIconPack()
                      ? settings.activeIconPack : "default"
            previewHeight: Math.min(width, Theme.itemSizeLarge * 1.5)
        }

        Label {
            width: parent.width
            height: Math.min(width, Theme.itemSizeLarge * 1.5)
            text: "Muoto"
            color: Theme.secondaryColor
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeLarge
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
        }
    }

    Image {
        enabled: settings.isRunning
        visible: settings.isRunning
        source: "image://theme/graphic-busyindicator-large"
        anchors.centerIn: parent
        fillMode: Image.PreserveAspectFit
        width: Theme.itemSizeLarge
        height: Theme.itemSizeLarge
        opacity: 0.6
        RotationAnimation on rotation {
            duration: 2000
            loops: Animation.Infinite
            running: settings.isRunning
            from: 0
            to: 360
        }
    }
}
