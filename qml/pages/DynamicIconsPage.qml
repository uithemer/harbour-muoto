import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page {
    id: dynPage

    DynamicIconsTabContent {
        id: dynTab
        anchors.fill: parent
        tabActive: dynPage.status === PageStatus.Active
    }

    BusyState { id: busyindicator }
}
