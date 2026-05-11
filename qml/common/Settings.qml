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
    }

    property alias wizardDone: conf.wizardDone
    property alias activeIconPack: conf.activeIconPack
    property alias activeFontPack: conf.activeFontPack

    property bool homeRefresh: true
    property bool isRunning: false

    function deactivateIcon() { activeIconPack = "default"; }
    function deactivateFont() { activeFontPack = "default"; }

    id: settings

    onWizardDoneChanged: conf.sync();
    onActiveIconPackChanged: conf.sync();
    onActiveFontPackChanged: conf.sync();

    Component.onCompleted: {
        conf.sync();
    }
}
