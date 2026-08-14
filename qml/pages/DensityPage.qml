import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: densityPage

    PageHeader {
        id: header
        title: qsTr("Display density")
    }

    DensityTabContent {
        id: densityTab
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        tabActive: densityPage.status === PageStatus.Active
    }

    BusyState { id: busyindicator }
}
