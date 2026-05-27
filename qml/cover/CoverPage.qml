import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0
import harbour.muoto 1.0
import "../components"

CoverBackground
{
    id: coverRoot

    // Set when the cover sync action starts an icon op; MainPage also
    // listens for iconsApplied and publishes from _finalise().
    property bool iconOpFromCover: false

    // Normalise /usr/share/<dir>/package lookup: accept bare pack ids
    // (what dconf stores) or full harbour-themepack-* dir names so we
    // never double-prefix when calling readThemePackName.
    function shareDirPackId(packId) {
        if (!packId || packId === "" || packId === "default")
            return "";
        var pref = "harbour-themepack-";
        if (packId.indexOf(pref) === 0)
            return packId;
        return pref + packId;
    }

    // Prefer the human name from the theme pack's package file; if that
    // file is missing or empty, fall back to a readable bare id so the
    // cover row is never blank while a non-default theme is active.
    function coverPackLabel(packId) {
        var dir = shareDirPackId(packId);
        if (!dir || dir === "")
            return "";
        var n = themepackmodel.readThemePackName(dir);
        if (n && n !== "")
            return n;
        var pref = "harbour-themepack-";
        if (dir.indexOf(pref) === 0)
            return dir.substring(pref.length).replace(/-/g, " ");
        return dir;
    }

     Notification { id: notification }
     // Cover sync re-runs ApplyIcons for the active pack (same as main UI
     // apply), using the overlay flag saved at last apply. ThemePackModel
     // is still needed because coverPackLabel() resolves the active
     // pack's display name via readThemePackName().
     ThemePackModel { id: themepackmodel }
     // 2.6.0: icon ops route through HelperClient and the daemon, so
     // listen for its bridged signals instead of iconapplier's local
     // ones (the GUI's IconApplier never does the privileged write
     // anymore, so its local apply/restore signals would never
     // fire from the privileged path).
     Connections {
         target: Helper
         onIconsApplied: {
             settings.isRunning = false;
             if (coverRoot.iconOpFromCover) {
                 coverRoot.iconOpFromCover = false;
                 notification.previewBody = qsTr("Settings applied.");
                 notification.publish();
             }
         }
         onIconsRestored: {
             settings.isRunning = false;
             if (coverRoot.iconOpFromCover) {
                 coverRoot.iconOpFromCover = false;
                 notification.publish();
             }
         }
         onError: {
             if (op === "ApplyIcons" || op === "RestoreIcons") {
                 notification.previewBody = message.length
                     ? message
                     : qsTr("Operation failed");
                 notification.publish();
                 settings.isRunning = false;
                 coverRoot.iconOpFromCover = false;
             }
         }
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
               ((settings.activeIconPack !== 'default') || (settings.activeFontPack !== 'default')) ? 0.1 : 0.3
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
        visible: !settings.isRunning
        CoverLabel {
            visible: (settings.activeIconPack && settings.activeIconPack !== 'default')
            icon: isLightTheme ? "../../images/icon.png" : "../../images/icon-light.png"
            label: coverRoot.coverPackLabel(settings.activeIconPack)
        }
        CoverLabel {
            visible: (settings.activeFontPack && settings.activeFontPack !== 'default')
            icon: isLightTheme ? "../../images/font.png" : "../../images/font-light.png"
            label: coverRoot.coverPackLabel(settings.activeFontPack)
        }
    }

    CoverActionList {
        iconBackground: true
        enabled: (settings.activeIconPack !== 'default') && !settings.isRunning
        CoverAction {
            iconSource: "image://theme/icon-cover-sync"
            onTriggered: {
                coverRoot.iconOpFromCover = true;
                settings.isRunning = true;
                Helper.applyIcons(settings.activeIconPack, true, settings.iconOverlay);
            }
        }
    }

}
