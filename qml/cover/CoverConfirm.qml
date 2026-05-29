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
     property int _fontPreviewRetries: 0

     Timer {
         id: fontPreviewRetry
         interval: 50
         repeat: true
         onTriggered: {
             var p = confirmPage()
             if (!p || !p.hasFont) {
                 stop()
                 _fontPreviewRetries = 0
                 return
             }
             if (p.previewFontBasename !== "") {
                 stop()
                 _fontPreviewRetries = 0
                 refreshCoverFontPreview()
                 return
             }
             if (++_fontPreviewRetries > 40) {
                 stop()
                 _fontPreviewRetries = 0
             }
         }
     }

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
         var basename = p.previewFontBasename
         if (p.packName && basename !== "") {
             fontPreviewRetry.stop()
             _fontPreviewRetries = 0
             fontloader.visible = true
             fontloader.reload()
         } else {
             fontloader.source = ""
             fontloader.visible = false
             if (p.hasFont && p.packName && !fontPreviewRetry.running) {
                 _fontPreviewRetries = 0
                 fontPreviewRetry.start()
             } else {
                 fontPreviewRetry.stop()
             }
         }
     }

     Component.onCompleted: {
         refreshCoverIconPreviewFromCache()
         refreshCoverFontPreview()
     }

     Connections {
         target: app
         onCoverIconPreviewSeqChanged: {
             refreshCoverIconPreview(app.coverIconPreviewPack,
                                     app.coverIconPreviewOk)
         }
         onCoverFontPreviewSeqChanged: refreshCoverFontPreview()
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
            source: app.isLightTheme ? "../../images/coverbg-light.png"
                                     : "../../images/coverbg-dark.png"
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
                var p = root.confirmPage()
                return p && p.packName && p.previewFontBasename !== ""
            }
            source: ""
            visible: false
            width: root.width
            height: root.height

            onActiveChanged: {
                if (active)
                    reload()
            }

            function reload() {
                source = ""
                var p = root.confirmPage()
                if (!p || !p.packName)
                    return
                var basename = p.previewFontBasename
                if (!basename)
                    return
                setSource("FontPreviewCover.qml", {
                    "packName": p.packName,
                    "selectedFont": basename
                })
            }
        }
    }

}
