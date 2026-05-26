import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0
import org.nemomobile.configuration 1.0
import harbour.uithemer 1.0
import "."

SilicaFlickable {
    id: densityView
    anchors.fill: parent
    contentHeight: densityContent.height
    enabled: !settings.isRunning
    opacity: settings.isRunning ? 0.2 : 1.0

    property bool _dpiRestoreQuiet: false

    function launcherIconSizeLabel(px) {
        if (!px || px < 1)
            return qsTr("System default")
        switch (px) {
        case 86: return qsTr("Compact (86)")
        case 108: return qsTr("Normal (108)")
        case 129: return qsTr("Medium (129)")
        case 151: return qsTr("Large (151)")
        case 172: return qsTr("Extra large (172)")
        default: return qsTr("Custom (%1)").arg(px)
        }
    }

    function syncDensityUi() {
        silica.sync()
        sldpr.value = silica.theme_pixel_ratio
    }

    Component.onCompleted: Helper.densityEnable()

    RemorsePopup { id: remorsepopup }
    ThemePack { id: themepack }
    Notification { id: notification }

    ThemePackModel {
        function applyDone() {
            notifyDone()
            if (settings.homeRefresh === true)
                themepack.restartHomescreen()
        }
        function notifyDone() {
            settings.isRunning = false
            notification.publish()
        }

        id: themepackmodel
        onDpiRestored: {
            syncDensityUi()
            if (densityView._dpiRestoreQuiet) {
                densityView._dpiRestoreQuiet = false
                return
            }
            applyDone()
        }
    }

    ConfigurationGroup {
        id: silica
        path: "/desktop/sailfish/silica"
        property real theme_pixel_ratio
        property real icon_size_launcher
    }

    Connections {
        target: settings
        onIsRunningChanged: {
            if (!settings.isRunning)
                syncDensityUi()
        }
    }

    Connections {
        target: silica
        onTheme_pixel_ratioChanged: sldpr.value = silica.theme_pixel_ratio
    }

    PullDownMenu {
        MenuItem {
            text: qsTr("About UI Themer")
            onClicked: pageStack.push(Qt.resolvedUrl("../pages/AboutPage.qml"))
        }

        MenuItem {
            text: qsTr("Restart homescreen")
            onClicked: remorsepopup.execute(qsTr("Restarting homescreen"), function() {
                themepack.restartHomescreen()
            })
        }

        MenuItem {
            text: qsTr("Restore display density")
            onClicked: {
                var dlgrestore = pageStack.push("../pages/RestoreDDPage.qml",
                                                 { "settings": settings })
                dlgrestore.accepted.connect(function() {
                    settings.isRunning = true
                    themepackmodel.restoreDpi(dlgrestore.restoreDPR,
                                              dlgrestore.restoreIconSize)
                })
            }
        }
    }

    Column {
        id: densityContent
        width: parent.width

        Grid {
            width: parent.width
            columns: isLandscape ? 2 : 1

            Column {
                width: isLandscape ? parent.width / 2 : parent.width

                SectionHeader { text: qsTr("Display scale") }

                Slider {
                    id: sldpr
                    width: parent.width
                    label: qsTr("Display scale")
                    maximumValue: 2.3
                    minimumValue: 0.7
                    stepSize: 0.05
                    value: silica.theme_pixel_ratio
                    valueText: value
                    onPressAndHold: cancel()

                    onReleased: {
                        silica.theme_pixel_ratio = value
                    }
                }

                LabelText {
                    text: qsTr("Controls how large Sailfish UI elements appear. "
                               + "Lower = more on screen; higher = larger text and buttons.")
                }
            }

            Column {
                width: isLandscape ? parent.width / 2 : parent.width

                SectionHeader { text: qsTr("Launcher icon size") }

                ComboBox {
                    id: cbiz
                    width: parent.width
                    label: qsTr("Launcher icon size")
                    description: qsTr("Icons on the home screen and app grid. "
                                      + "System default uses your device's normal size "
                                      + "(often 108 on many phones).")
                    value: densityView.launcherIconSizeLabel(silica.icon_size_launcher)

                    menu: ContextMenu {
                        MenuItem {
                            text: qsTr("System default")
                            onClicked: {
                                densityView._dpiRestoreQuiet = true
                                themepackmodel.restoreDpi(false, true)
                            }
                        }
                        MenuItem {
                            text: qsTr("Compact (86)")
                            onClicked: silica.icon_size_launcher = 86
                        }
                        MenuItem {
                            text: qsTr("Normal (108)")
                            onClicked: silica.icon_size_launcher = 108
                        }
                        MenuItem {
                            text: qsTr("Medium (129)")
                            onClicked: silica.icon_size_launcher = 129
                        }
                        MenuItem {
                            text: qsTr("Large (151)")
                            onClicked: silica.icon_size_launcher = 151
                        }
                        MenuItem {
                            text: qsTr("Extra large (172)")
                            onClicked: silica.icon_size_launcher = 172
                        }
                    }
                }

                LabelText {
                    text: qsTr("Pull down and tap Restart homescreen after changing these settings.")
                }
            }
        }

        Item {
            width: parent.width
            height: Theme.paddingLarge
        }
    }

    VerticalScrollDecorator { }
}
