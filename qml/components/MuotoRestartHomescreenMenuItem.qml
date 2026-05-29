import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0

MenuItem {
    property RemorsePopup remorsePopup
    property ThemePack themePack

    text: qsTr("Restart homescreen")
    onClicked: remorsePopup.execute(qsTr("Restarting homescreen"), function() {
        themePack.restartHomescreen()
    })
}
