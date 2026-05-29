import QtQuick 2.0
import Sailfish.Silica 1.0
import "../common"
import "."

Column {
    width: parent.width

    property Settings settings
    property string explanation: ""
    property alias homeRefreshSwitch: tshomerefresh

    SectionHeader { text: qsTr("Restart homescreen") }

    LabelText {
        text: explanation
    }

    TextSwitch {
        id: tshomerefresh
        text: qsTr("Restart homescreen automatically")
        checked: settings.homeRefresh
        onCheckedChanged: settings.homeRefresh = checked
    }
}
