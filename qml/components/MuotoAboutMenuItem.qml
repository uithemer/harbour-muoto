import QtQuick 2.0
import Sailfish.Silica 1.0

MenuItem {
    text: qsTr("About Muoto")
    onClicked: pageStack.push(Qt.resolvedUrl("../pages/AboutPage.qml"))
}
