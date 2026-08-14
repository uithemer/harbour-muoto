import QtQuick 2.0
import Nemo.Configuration 1.0

Item
{

    ConfigurationGroup {
        id: conf
        path: "/apps/harbour-muoto"
        property bool wizardDone
        property string activeIconPack: "default"
        property string activeFontPack: "default"
        property string activeFontWeight: ""
        // Mirror of the user's "apply icon overlay" choice at the last
        // ApplyIcons(pack, runPack, overlay). Cover sync and main-page apply pass
        // runPack/overlay so overlay-composited icons are included when
        // the user opted in. Cleared on restore so it cannot leak across
        // theme generations.
        property bool iconOverlay
        // Optional full homescreen restart after apply/restore (default on).
        property bool homeRefresh: true
    }

    ConfigurationGroup {
        id: launcherConf
        path: "/apps/harbour-muoto/launcher"
        property bool dynamicClockEnabled: true
        property bool dynamicCalendarEnabled: true
    }

    property alias wizardDone: conf.wizardDone
    property alias activeIconPack: conf.activeIconPack
    property alias activeFontPack: conf.activeFontPack
    property alias activeFontWeight: conf.activeFontWeight
    property alias iconOverlay: conf.iconOverlay
    property alias homeRefresh: conf.homeRefresh
    property alias dynamicClockEnabled: launcherConf.dynamicClockEnabled
    property alias dynamicCalendarEnabled: launcherConf.dynamicCalendarEnabled

    property bool isRunning: false

    function deactivateIcon() { activeIconPack = "default"; }
    function deactivateFont() {
        activeFontPack = "default"
        activeFontWeight = ""
    }

    function packIdIsActive(packId) {
        return packId && packId !== "" && packId !== "default"
    }
    function hasActiveIconPack() { return packIdIsActive(activeIconPack) }
    function hasActiveFontPack() { return packIdIsActive(activeFontPack) }

    function syncToDisk() {
        conf.sync()
        launcherConf.sync()
    }

    id: settings

    onWizardDoneChanged: conf.sync();
    onActiveIconPackChanged: conf.sync();
    onActiveFontPackChanged: conf.sync();
    onActiveFontWeightChanged: conf.sync();
    onIconOverlayChanged: conf.sync();
    onHomeRefreshChanged: conf.sync();
    onDynamicClockEnabledChanged: launcherConf.sync();
    onDynamicCalendarEnabledChanged: launcherConf.sync();

    Component.onCompleted: {
        conf.sync();
        if (!packIdIsActive(activeIconPack))
            activeIconPack = "default";
        if (!packIdIsActive(activeFontPack))
            activeFontPack = "default";
    }
}
