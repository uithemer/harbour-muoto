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
        // ApplyIcons call. The GUI's auto-theming watcher passes this
        // to Helper.themeNewDesktops(overlay) so newly-installed apps
        // that the pack has no asset for still get an overlay-
        // composited icon when the user opted in. Cleared on restore
        // so it cannot leak across theme generations.
        property bool iconOverlay
    }

    property alias wizardDone: conf.wizardDone
    property alias activeIconPack: conf.activeIconPack
    property alias activeFontPack: conf.activeFontPack
    property alias iconOverlay: conf.iconOverlay

    // Off by default: icon apply/restore triggers launcher refresh via
    // Optional full homescreen restart fallback (default off).
    property bool homeRefresh: false
    property bool isRunning: false

    function deactivateIcon() { activeIconPack = "default"; }
    function deactivateFont() { activeFontPack = "default"; }

    id: settings

    onWizardDoneChanged: conf.sync();
    onActiveIconPackChanged: conf.sync();
    onActiveFontPackChanged: conf.sync();
    onIconOverlayChanged: conf.sync();

    Component.onCompleted: {
        conf.sync();
    }
}
