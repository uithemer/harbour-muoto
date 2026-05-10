import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0
import harbour.uithemer 1.0
import "../components"

CoverBackground
{
     Notification { id: notification }
     ThemePackModel {
         function notifyDone() {
             settings.isRunning = false;
             notification.publish();
         }
         id: themepackmodel
         onOcrRestored: notifyDone()
     }
     Connections {
         target: iconapplier
         onReasserted: { settings.isRunning = false; notification.publish(); }
         onRestored: { settings.isRunning = false; notification.publish(); }
     }
     ThemePack {
         function notifyDone() {
             settings.isRunning = false;
             notification.publish();
         }
         id: themepack
         onHomescreenRestarted: notifyDone()
     }

    Rectangle {
        anchors.fill: parent
        color: "transparent"

    Image {
        id: coverimg
        fillMode: Image.PreserveAspectFit
        source: isLightTheme ? "../../images/coverbg.png" : "../../images/coverbg-light.png"
        opacity: {
            if (settings.isRunning)
               0.1
            else
               (settings.coverActiveTheme) && ((settings.activeIconPack !== 'default') || (settings.activeFontPack !== 'default')) ? 0.1 : 0.3
        }
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width
        height: sourceSize.height * width / sourceSize.width
    }

    Image {
        id: refreshimg
        enabled: settings.isRunning
        visible: settings.isRunning
        source: "image://theme/graphic-busyindicator-large"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        fillMode: Image.PreserveAspectFit
        width: Theme.itemSizeLarge
        height: Theme.itemSizeLarge
        opacity: 0.6
        RotationAnimation on rotation {
            duration: 2000;
            loops: Animation.Infinite;
            running: settings.isRunning
            from: 0; to: 360
        }
    }

     }

    Column {
        spacing: Theme.paddingSmall
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Theme.paddingSmall
        anchors.rightMargin: Theme.paddingSmall
        anchors.top: parent.top
        anchors.topMargin: Theme.paddingLarge
        visible: (settings.coverActiveTheme && !settings.isRunning)
        CoverLabel {
            visible: (settings.activeIconPack !== 'default')
            icon: isLightTheme ? "../../images/icon.png" : "../../images/icon-light.png"
            label: themepackmodel.readThemePackName("harbour-themepack-" + settings.activeIconPack)
        }
        CoverLabel {
            visible: (settings.activeFontPack !== 'default')
            icon: isLightTheme ? "../../images/font.png" : "../../images/font-light.png"
            label: themepackmodel.readThemePackName("harbour-themepack-" + settings.activeFontPack)
        }
    }

    Loader {
        source: {
            if (settings.coverAction1 !== 3 && settings.coverAction2 !== 3)
            return Qt.resolvedUrl("CoverActionList2.qml")
            else
            Qt.resolvedUrl("CoverActionList1.qml")
        }
    }

}
