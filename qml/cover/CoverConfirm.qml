import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0
import harbour.uithemer 1.0
import "../components"

CoverBackground
{
    id: root
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
     onStatusChanged: {
         if (status === Cover.Active || Cover.Activating || Cover.Deactivating) {
             if (pageStack.currentPage.hasIcons || pageStack.currentPage.hasIconOverlay) {
                 var pn = pageStack.currentPage.packName
                 if (pn) {
                     coverimgpreview.source = ""
                     coverimgpreview.source = "image://uithemer/preview/" + pn + "?t=" + Date.now()
                 }
             }
             if (pageStack.currentPage.hasFont && pageStack.currentPage.selectedFont !== "") {
                 fontloader.visible = true
                 fontloader.reload()
             }

         }
     }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        anchors.topMargin: Theme.paddingMedium

        Label {
            width: root.width - (x * 2)
            anchors.top: parent.top
            anchors.topMargin: Theme.paddingSmall
            x: Theme.paddingLarge
            font.pixelSize: Theme.fontSizeMedium
            truncationMode: TruncationMode.Fade
            text: pageStack.currentPage.confirmheadername
        }

        Image {
            id: coverimg
            fillMode: Image.PreserveAspectFit
            source: isLightTheme ? "../../images/coverbg.png" : "../../images/coverbg-light.png"
            opacity: 0.1
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            height: sourceSize.height * width / sourceSize.width
        }

        Image {
            id: coverimgpreview
            opacity: 0.8
            fillMode: Image.PreserveAspectFit
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - Theme.paddingMedium
            height: sourceSize.height * width / sourceSize.width
            asynchronous: true
            cache: false
        }

    }

    Column {
        spacing: Theme.paddingSmall
        anchors.top: parent.top
        anchors.topMargin: parent.height/5
        anchors.verticalCenter: parent.verticalCenter

        Loader {
            id: fontloader
            source: "FontPreviewCover.qml"
            visible: false
            width: root.width
            height: root.height

            function reload() {
                source = ""
                source = "FontPreviewCover.qml"
            }
        }
    }

}
