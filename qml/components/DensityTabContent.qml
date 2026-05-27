import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0
import org.nemomobile.configuration 1.0
import harbour.muoto 1.0
import "."

SilicaFlickable {
    id: densityView
    anchors.fill: parent
    contentHeight: densityContent.height
    enabled: !settings.isRunning
    opacity: settings.isRunning ? 0.2 : 1.0

    property bool _dpiRestoreQuiet: false
    property bool densityReady: false

    function requestDensityUnlock() {
        densityReady = false
        Helper.densityEnable()
    }

    // ComboBox display comes from currentIndex/currentItem; index from ConfigurationValue
    // so an unset key (after dconf reset) maps to "System default", not vendor default 108.
    function iconSizeMenuIndex() {
        iconSizeLauncherKey.sync()
        var v = iconSizeLauncherKey.value
        if (v === undefined)
            return 0
        var px = Number(v)
        if (isNaN(px) || px < 1)
            return 0
        switch (px) {
        case 86: return 1
        case 108: return 2
        case 129: return 3
        case 151: return 4
        case 172: return 5
        default: return -1
        }
    }

    function syncIconSizeCombo() {
        iconSizeComboSyncTimer.targetIndex = iconSizeMenuIndex()
        iconSizeComboSyncTimer.restart()
    }

    function syncDensityUi() {
        silica.sync()
        iconSizeLauncherKey.sync()
        sldpr.value = silica.theme_pixel_ratio
        syncIconSizeCombo()
    }

    Timer {
        id: iconSizeComboSyncTimer
        interval: 1
        property int targetIndex: 0
        onTriggered: {
            if (targetIndex < 0) {
                cbiz.currentIndex = -1
                return
            }
            // Force refresh when the index is unchanged (Silica skips no-op updates).
            if (cbiz.currentIndex === targetIndex)
                cbiz.currentIndex = -1
            cbiz.currentIndex = targetIndex
        }
    }

    Component.onCompleted: requestDensityUnlock()

    onVisibleChanged: {
        if (visible && !densityReady)
            requestDensityUnlock()
    }

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

    ConfigurationValue {
        id: iconSizeLauncherKey
        key: "/desktop/sailfish/silica/icon_size_launcher"
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
        onIcon_size_launcherChanged: densityView.syncIconSizeCombo()
        onValueChanged: function(key) {
            if (key === "icon_size_launcher")
                densityView.syncIconSizeCombo()
        }
    }

    Connections {
        target: iconSizeLauncherKey
        onValueChanged: densityView.syncIconSizeCombo()
    }

    Connections {
        target: Helper
        onDensityEnabled: {
            densityView.densityReady = true
            densityView.syncDensityUi()
        }
        onError: {
            if (op !== "DensityEnable")
                return
            densityView.densityReady = false
            notification.previewBody = message.length
                ? message
                : qsTr("Could not unlock display density settings")
            notification.publish()
        }
    }

    PullDownMenu {
        MenuItem {
            text: qsTr("About Muoto")
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
                    enabled: densityView.densityReady
                    maximumValue: 2.3
                    minimumValue: 0.7
                    stepSize: 0.05
                    value: silica.theme_pixel_ratio
                    valueText: value
                    onPressAndHold: cancel()

                    onReleased: {
                        if (!densityView.densityReady)
                            return
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
                    enabled: densityView.densityReady
                    description: qsTr("Icons on the home screen and app grid. "
                                      + "System default uses your device's normal size "
                                      + "(often 108 on many phones).")

                    Component.onCompleted: densityView.syncIconSizeCombo()

                    menu: ContextMenu {
                        MenuItem {
                            text: qsTr("System default")
                            enabled: densityView.densityReady
                            onClicked: {
                                densityView._dpiRestoreQuiet = true
                                themepackmodel.restoreDpi(false, true)
                            }
                        }
                        MenuItem {
                            text: qsTr("Compact (86)")
                            enabled: densityView.densityReady
                            onClicked: silica.icon_size_launcher = 86
                        }
                        MenuItem {
                            text: qsTr("Normal (108)")
                            enabled: densityView.densityReady
                            onClicked: silica.icon_size_launcher = 108
                        }
                        MenuItem {
                            text: qsTr("Medium (129)")
                            enabled: densityView.densityReady
                            onClicked: silica.icon_size_launcher = 129
                        }
                        MenuItem {
                            text: qsTr("Large (151)")
                            enabled: densityView.densityReady
                            onClicked: silica.icon_size_launcher = 151
                        }
                        MenuItem {
                            text: qsTr("Extra large (172)")
                            enabled: densityView.densityReady
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
