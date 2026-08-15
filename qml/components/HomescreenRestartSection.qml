import "."
import "../common"
import QtQuick 2.0
import Sailfish.Silica 1.0

Column {
    property Settings settings
    property string explanation: ""
    property alias homeRefreshSwitch: tshomerefresh

    width: parent.width

    SectionHeader {
        text: qsTr("Restart homescreen")
    }

    MuotoTextLabel {
        text: explanation
    }

    TextSwitch {
        id: tshomerefresh

        text: qsTr("Restart homescreen automatically")
        checked: settings.homeRefresh
        onCheckedChanged: settings.homeRefresh = checked
    }

}
