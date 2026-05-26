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
            silica.sync()
            sldpr.value = silica.theme_pixel_ratio
            cbiz.value = silica.icon_size_launcher
            applyDone()
        }
    }

    ConfigurationGroup {
        id: silica
        path: "/desktop/sailfish/silica"
        property real theme_pixel_ratio
        property real icon_size_launcher
    }

    PullDownMenu {
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

                SectionHeader { text: qsTr("Device pixel ratio") }

                Slider {
                    id: sldpr
                    width: parent.width
                    label: qsTr("Device pixel ratio")
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
                    text: qsTr("Change the display pixel ratio. To a smaller value corresponds an higher density.")
                }
            }

            Column {
                width: isLandscape ? parent.width / 2 : parent.width

                SectionHeader { text: qsTr("Icon size") }

                ComboBox {
                    id: cbiz
                    width: parent.width
                    label: qsTr("Icon size")
                    description: qsTr("Change the size of UI icons. To a greater value corresponds an huger size.")
                    value: silica.icon_size_launcher

                    menu: ContextMenu {
                        MenuItem { text: "86"; onClicked: silica.icon_size_launcher = 86 }
                        MenuItem { text: "108"; onClicked: silica.icon_size_launcher = 108 }
                        MenuItem { text: "129"; onClicked: silica.icon_size_launcher = 129 }
                        MenuItem { text: "151"; onClicked: silica.icon_size_launcher = 151 }
                        MenuItem { text: "172"; onClicked: silica.icon_size_launcher = 172 }
                    }
                }

                LabelText {
                    text: qsTr("Remember to restart the homescreen from the pulley menu on the Themes tab after changing these settings.")
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
