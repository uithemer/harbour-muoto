import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0
import "homescreenRestart.js" as HomescreenRestart

MenuItem {
    property RemorsePopup remorsePopup
    property ThemePack themePack

    text: qsTr("Restart homescreen")
    onClicked: HomescreenRestart.restartWithRemorse(remorsePopup, themePack, qsTr("Restarting homescreen"))
}
