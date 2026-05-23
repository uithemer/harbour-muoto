import QtQuick 2.0
import harbour.uithemer 1.0
import org.nemomobile.configuration 1.0

Item
{

    ConfigurationGroup {
        id: conf
        path: "/desktop/lipstick/sailfishos-uithemer"
        property bool wizardDone
        property string activeIconPack
        property string activeFontPack
        // Mirror of the user's "apply icon overlay" choice at the last
        // ApplyIcons(pack, runPack, overlay). Cover sync and main-page apply pass
        // runPack/overlay so overlay-composited icons are included when
        // the user opted in. Cleared on restore so it cannot leak across
        // theme generations.
        property bool iconOverlay
        // Optional full homescreen restart fallback (default off).
        property bool homeRefresh
    }

    property alias wizardDone: conf.wizardDone
    property alias activeIconPack: conf.activeIconPack
    property alias activeFontPack: conf.activeFontPack
    property alias iconOverlay: conf.iconOverlay
    property alias homeRefresh: conf.homeRefresh

    property bool isRunning: false

    function deactivateIcon() { activeIconPack = "default"; }
    function deactivateFont() { activeFontPack = "default"; }

    function syncToDisk() { conf.sync(); }

    id: settings

    onWizardDoneChanged: conf.sync();
    onActiveIconPackChanged: conf.sync();
    onActiveFontPackChanged: conf.sync();
    onIconOverlayChanged: conf.sync();
    onHomeRefreshChanged: conf.sync();

    Component.onCompleted: {
        conf.sync();
    }
}
