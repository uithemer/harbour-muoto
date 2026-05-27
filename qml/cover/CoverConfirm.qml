import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0
import harbour.muoto 1.0
import "../components"

CoverBackground
{
    id: root
     Notification { id: notification }
     // 2.6.0: icon ops route through HelperClient and the daemon, so
     // listen for its bridged signals instead of iconapplier's local
     // ones.
     Connections {
         target: Helper
         onIconsRestored: { settings.isRunning = false; notification.publish(); }
         onError: {
             if (op === "RestoreIcons") {
                 settings.isRunning = false;
             }
         }
     }
     ThemePack {
         function notifyDone() {
             settings.isRunning = false;
             notification.publish();
         }
         id: themepack
         onHomescreenRestarted: notifyDone()
     }

     function confirmPage() {
         var p = pageStack.currentPage
         return p && p.confirmheadername !== undefined ? p : null
     }

     property string _coverPreviewPack: ""

     function setCoverPreviewSource(pack) {
         // Image.source is a QUrl — no .indexOf(); only skip when already showing this pack.
         if (_coverPreviewPack === pack && coverimgpreview.status === Image.Ready)
             return
         _coverPreviewPack = pack
         coverimgpreview.source = "image://muoto/preview/" + pack + "?t=" + Date.now()
     }

     function refreshCoverIconPreviewFromCache() {
         var p = confirmPage()
         if (!p)
             return
         if (!(p.hasIcons || p.hasIconOverlay) || !p.packName)
             return
         setCoverPreviewSource(p.packName)
     }

     function refreshCoverIconPreview(previewPack, ok) {
         var p = confirmPage()
         if (!p || previewPack !== p.packName)
             return
         if (ok)
             setCoverPreviewSource(previewPack)
         else {
             _coverPreviewPack = ""
             coverimgpreview.source = ""
         }
     }

     function refreshCoverFontPreview() {
         var p = confirmPage()
         if (!p)
             return
         if (p.hasFont && p.selectedFont !== "") {
             fontloader.visible = true
             fontloader.reload()
         }
     }

     Connections {
         target: app
         onCoverIconPreviewSeqChanged: {
             refreshCoverIconPreview(app.coverIconPreviewPack,
                                     app.coverIconPreviewOk)
         }
     }

     Connections {
         target: Qt.application
         onStateChanged: {
             if (state !== Qt.ApplicationActive
                     && app.coverMode === "confirmDialog") {
                 refreshCoverIconPreviewFromCache()
                 refreshCoverFontPreview()
             }
         }
     }

     onStatusChanged: {
         if (status === Cover.Active || Cover.Activating || Cover.Deactivating) {
             refreshCoverIconPreviewFromCache()
             refreshCoverFontPreview()
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
            text: {
                var p = pageStack.currentPage
                return (p && p.confirmheadername !== undefined)
                       ? p.confirmheadername : ""
            }
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
            active: {
                var p = pageStack.currentPage
                if (!p)
                    return false
                return p.hasFont === true || p.hasFontNonLatin === true
            }
            source: ""
            visible: false
            width: root.width
            height: root.height

            function reload() {
                source = ""
                if (pageStack.currentPage.hasFont || pageStack.currentPage.hasFontNonLatin)
                    source = "FontPreviewCover.qml"
            }
        }
    }

}
