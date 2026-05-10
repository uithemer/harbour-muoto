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
        property int coverAction1
        property int coverAction2
        property int autoUpdate
        property bool servicesu
    }

    property alias wizardDone: conf.wizardDone
    property alias activeIconPack: conf.activeIconPack
    property alias activeFontPack: conf.activeFontPack
    property alias coverAction1: conf.coverAction1
    property alias coverAction2: conf.coverAction2
    property alias autoUpdate: conf.autoUpdate
    property alias servicesu: conf.servicesu

    property bool homeRefresh: true
    property bool isRunning: false

    function deactivateIcon() { activeIconPack = "default"; }
    function deactivateFont() { activeFontPack = "default"; }

    id: settings

    onWizardDoneChanged: conf.sync();
    onActiveIconPackChanged: conf.sync();
    onActiveFontPackChanged: conf.sync();
    onCoverAction1Changed: conf.sync();
    onCoverAction2Changed: conf.sync();
    onAutoUpdateChanged: conf.sync();
    onServicesuChanged: conf.sync();

    Component.onCompleted: {
        conf.sync();
    }
}
