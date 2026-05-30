import QtQuick 2.0
import Sailfish.Silica 1.0
import Opal.Tabs 1.0 as T
import "../components"

Page
{
    id: mainpage

    onStatusChanged: {
        if (status === PageStatus.Active)
            app.coverMode = "mainPage"
    }

    T.TabView {
        id: tabs
        anchors.fill: parent
        tabBarPosition: Qt.AlignTop
        enabled: !settings.isRunning
        interactive: !settings.isRunning

        T.Tab {
            title: qsTr("Themes")

            Component {
                T.TabItem {
                    flickable: themesTab

                    ThemesTabContent {
                        id: themesTab
                        anchors.fill: parent
                        tabActive: tabs.currentIndex === 0
                    }
                }
            }
        }

        T.Tab {
            title: qsTr("Display density")

            Component {
                T.TabItem {
                    flickable: densityTab

                    DensityTabContent {
                        id: densityTab
                        tabActive: tabs.currentIndex === 1
                    }
                }
            }
        }
    }

    BusyState { id: busyindicator }
}
