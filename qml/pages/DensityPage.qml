import Nemo.Configuration 1.0
import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0
import "../common"
import "../components"

Dialog {
    id: dlg

    property var themeWork
    property Settings settings

    property bool densityReady: false
    property real vendorDpr: 0
    property bool vendorDprKnown: false

    property bool appliedSnapshotReady: false
    property real appliedDpr: 0
    property int appliedIconIndex: 0

    readonly property bool dprAtDefault: vendorDprKnown
                                         && Math.abs(sldpr.value - vendorDpr) < 0.001
    readonly property bool dirty: appliedSnapshotReady
                                  && (Math.abs(sldpr.value - appliedDpr) >= 0.001
                                      || cbiz.currentIndex !== appliedIconIndex)

    canAccept: densityReady && dirty

    function requestDensityUnlock() {
        densityReady = false
        Helper.densityEnable()
    }

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

    function iconSizePx() {
        switch (cbiz.currentIndex) {
        case 1: return 86
        case 2: return 108
        case 3: return 129
        case 4: return 151
        case 5: return 172
        default: return -1
        }
    }

    readonly property int previewIconPx: {
        var px = iconSizePx()
        if (px > 0)
            return px
        return Theme.iconSizeLauncher
    }
    readonly property real previewFontScale: {
        var pr = Theme.pixelRatio
        if (pr <= 0)
            return 1
        return sldpr.value / pr
    }

    function syncIconSizeCombo() {
        if (!densityReady)
            return
        iconSizeComboSyncTimer.targetIndex = iconSizeMenuIndex()
        iconSizeComboSyncTimer.restart()
    }

    function syncDensityUi() {
        silica.sync()
        iconSizeLauncherKey.sync()
        sldpr.value = silica.theme_pixel_ratio
        syncIconSizeCombo()
    }

    function restoreDefaultDpr() {
        if (!vendorDprKnown)
            return
        sldpr.value = vendorDpr
    }

    Timer {
        id: iconSizeComboSyncTimer
        interval: 1
        property int targetIndex: 0
        onTriggered: {
            if (targetIndex < 0) {
                cbiz.currentIndex = -1
            } else {
                if (cbiz.currentIndex === targetIndex)
                    cbiz.currentIndex = -1
                cbiz.currentIndex = targetIndex
            }
            dlg.appliedDpr = sldpr.value
            dlg.appliedIconIndex = cbiz.currentIndex
            dlg.appliedSnapshotReady = true
        }
    }

    function loadVendorDpr() {
        var xhr = new XMLHttpRequest()
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return
            var t = xhr.responseText || ""
            var m = t.match(/theme_pixel_ratio\s*=\s*([0-9]*\.?[0-9]+)/)
            if (m) {
                dlg.vendorDpr = Number(m[1])
                dlg.vendorDprKnown = dlg.vendorDpr > 0
                return
            }
            if (Theme.pixelRatio > 0) {
                dlg.vendorDpr = Theme.pixelRatio
                dlg.vendorDprKnown = true
            }
        }
        xhr.open("GET", "file:///etc/dconf/db/vendor.d/silica-configs.txt")
        xhr.send()
    }

    Component.onCompleted: {
        loadVendorDpr()
        requestDensityUnlock()
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
        target: Helper
        onDensityEnabled: {
            dlg.densityReady = true
            dlg.syncDensityUi()
        }
        onError: {
            if (op !== "DensityEnable")
                return
            dlg.densityReady = false
            app.showHelperError(message,
                qsTr("Could not unlock display density settings"))
        }
    }

    onAccepted: {
        settings.homeRefresh = restartSection.homeRefreshSwitch.checked
        var resetDpr = dprAtDefault
        var resetIcon = cbiz.currentIndex === 0
        if (!resetDpr)
            silica.theme_pixel_ratio = sldpr.value
        var px = iconSizePx()
        if (!resetIcon && px > 0)
            silica.icon_size_launcher = px
        if (resetDpr || resetIcon)
            themeWork.restoreDpi(resetDpr, resetIcon)
        else
            themeWork.finishDensityApply()
    }

    BusyState { id: busyindicator }

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentHeight: content.height
        enabled: !settings.isRunning
        opacity: settings.isRunning ? 0.2 : 1.0

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width

            DialogHeader {
                dialog: dlg
                cancelText: qsTr("Cancel")
                acceptText: qsTr("Apply")
            }

            Grid {
                width: parent.width
                columns: isLandscape ? 2 : 1

                Column {
                    width: isLandscape ? parent.width / 2 : parent.width

                    Item {
                        id: densityPreviewHost
                        width: parent.width
                        height: Math.min(parent.width, Math.max(280, flickable.height * 0.32))

                        StockLauncherIcons { id: stockIcons }

                        Column {
                            anchors.centerIn: parent
                            spacing: Theme.paddingSmall
                            width: parent.width

                            Image {
                                width: dlg.previewIconPx
                                height: width
                                anchors.horizontalCenter: parent.horizontalCenter
                                asynchronous: true
                                cache: true
                                fillMode: Image.PreserveAspectFit
                                source: stockIcons.count > 0
                                        ? stockIcons.get(0, "fileURL")
                                        : "image://theme/icon-launcher-application"
                            }

                            Label {
                                width: parent.width
                                horizontalAlignment: Text.AlignHCenter
                                text: qsTr("Aa Bb")
                                color: Theme.primaryColor
                                font.pixelSize: Theme.fontSizeMedium * dlg.previewFontScale
                            }
                        }
                    }

                    SectionHeader { text: qsTr("Display scale") }

                    Slider {
                        id: sldpr
                        width: parent.width
                        label: qsTr("Display scale")
                        enabled: dlg.densityReady
                        maximumValue: 2.3
                        minimumValue: 0.7
                        stepSize: 0.05
                        valueText: value
                        onPressAndHold: cancel()
                    }


            Item {
                width: parent.width
                height: Theme.paddingLarge
            }
            
                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: Math.min(parent.width - Theme.paddingLarge * 2,
                                        Theme.buttonWidthMedium)
                        text: qsTr("Restore default")
                        enabled: dlg.densityReady && dlg.vendorDprKnown && !dlg.dprAtDefault
                        onClicked: dlg.restoreDefaultDpr()
                    }


            Item {
                width: parent.width
                height: Theme.paddingLarge
            }

                    MuotoTextLabel {
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
                        enabled: dlg.densityReady
                        description: qsTr("Icons on the home screen and app grid. "
                                          + "System default uses your device's normal size.")

                        menu: ContextMenu {
                            MenuItem { text: qsTr("System default") }
                            MenuItem { text: qsTr("Compact (86)") }
                            MenuItem { text: qsTr("Normal (108)") }
                            MenuItem { text: qsTr("Medium (129)") }
                            MenuItem { text: qsTr("Large (151)") }
                            MenuItem { text: qsTr("Extra large (172)") }
                        }
                    }

                    MuotoTextLabel {
                        text: qsTr("When you are done, restart the homescreen to apply these changes.")
                    }
                }
            }

            HomescreenRestartSection {
                id: restartSection
                settings: dlg.settings
                explanation: qsTr("Restart the homescreen after applying display density so all apps pick up the changes.")
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }
        }
    }
}
