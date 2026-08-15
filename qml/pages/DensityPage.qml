import "../common"
import "../components"
import Nemo.Configuration 1.0
import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0

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
    readonly property bool dprAtDefault: vendorDprKnown && Math.abs(sldpr.value - vendorDpr) < 0.001
    readonly property bool dirty: appliedSnapshotReady && (Math.abs(sldpr.value - appliedDpr) >= 0.001 || cbiz.currentIndex !== appliedIconIndex)
    readonly property int previewIconPx: {
        var base = iconSizePx();
        if (base <= 0)
            base = Theme.iconSizeLauncher;

        return Math.max(1, Math.round(base * previewFontScale));
    }
    readonly property real previewFontScale: {
        var pr = Theme.pixelRatio;
        if (pr <= 0)
            return 1;

        return sldpr.value / pr;
    }

    function requestDensityUnlock() {
        densityReady = false;
        Helper.densityEnable();
    }

    function iconSizeMenuIndex() {
        iconSizeLauncherKey.sync();
        var v = iconSizeLauncherKey.value;
        if (v === undefined)
            return 0;

        var px = Number(v);
        if (isNaN(px) || px < 1)
            return 0;

        switch (px) {
        case 86:
            return 1;
        case 108:
            return 2;
        case 129:
            return 3;
        case 151:
            return 4;
        case 172:
            return 5;
        default:
            return -1;
        }
    }

    function iconSizePx() {
        switch (cbiz.currentIndex) {
        case 1:
            return 86;
        case 2:
            return 108;
        case 3:
            return 129;
        case 4:
            return 151;
        case 5:
            return 172;
        default:
            return -1;
        }
    }

    function syncIconSizeCombo() {
        if (!densityReady)
            return ;

        iconSizeComboSyncTimer.targetIndex = iconSizeMenuIndex();
        iconSizeComboSyncTimer.restart();
    }

    function syncDensityUi() {
        silica.sync();
        iconSizeLauncherKey.sync();
        sldpr.value = silica.theme_pixel_ratio;
        syncIconSizeCombo();
    }

    function restoreDefaultDpr() {
        if (!vendorDprKnown)
            return ;

        sldpr.value = vendorDpr;
    }

    function loadVendorDpr() {
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE)
                return ;

            var t = xhr.responseText || "";
            var m = t.match(/theme_pixel_ratio\s*=\s*([0-9]*\.?[0-9]+)/);
            if (m) {
                dlg.vendorDpr = Number(m[1]);
                dlg.vendorDprKnown = dlg.vendorDpr > 0;
                return ;
            }
            if (Theme.pixelRatio > 0) {
                dlg.vendorDpr = Theme.pixelRatio;
                dlg.vendorDprKnown = true;
            }
        };
        xhr.open("GET", "file:///etc/dconf/db/vendor.d/silica-configs.txt");
        xhr.send();
    }

    canAccept: densityReady && dirty
    Component.onCompleted: {
        loadVendorDpr();
        requestDensityUnlock();
    }
    onAccepted: {
        settings.homeRefresh = restartSection.homeRefreshSwitch.checked;
        var resetDpr = dprAtDefault;
        var resetIcon = cbiz.currentIndex === 0;
        if (!resetDpr)
            silica.theme_pixel_ratio = sldpr.value;

        var px = iconSizePx();
        if (!resetIcon && px > 0)
            silica.icon_size_launcher = px;

        if (resetDpr || resetIcon)
            themeWork.restoreDpi(resetDpr, resetIcon);
        else
            themeWork.finishDensityApply();
    }

    Timer {
        id: iconSizeComboSyncTimer

        property int targetIndex: 0

        interval: 1
        onTriggered: {
            if (targetIndex < 0) {
                cbiz.currentIndex = -1;
            } else {
                if (cbiz.currentIndex === targetIndex)
                    cbiz.currentIndex = -1;

                cbiz.currentIndex = targetIndex;
            }
            dlg.appliedDpr = sldpr.value;
            dlg.appliedIconIndex = cbiz.currentIndex;
            dlg.appliedSnapshotReady = true;
        }
    }

    ConfigurationGroup {
        id: silica

        property real theme_pixel_ratio
        property real icon_size_launcher

        path: "/desktop/sailfish/silica"
    }

    ConfigurationValue {
        id: iconSizeLauncherKey

        key: "/desktop/sailfish/silica/icon_size_launcher"
    }

    Connections {
        target: Helper
        onDensityEnabled: {
            dlg.densityReady = true;
            dlg.syncDensityUi();
        }
        onError: {
            if (op !== "DensityEnable")
                return ;

            dlg.densityReady = false;
            app.showHelperError(message, qsTr("Could not unlock display density settings"));
        }
    }

    BusyState {
        id: busyindicator
    }

    SilicaFlickable {
        id: flickable

        anchors.fill: parent
        contentHeight: content.height
        enabled: !settings.isRunning
        opacity: settings.isRunning ? 0.2 : 1

        VerticalScrollDecorator {
        }

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

                        DensityPreview {
                            anchors.fill: parent
                            iconPx: {
                                var cap = densityPreviewHost.height * 0.55;
                                if (cap > 1)
                                    return Math.min(dlg.previewIconPx, Math.round(cap));

                                return dlg.previewIconPx;
                            }
                            fontScale: dlg.previewFontScale
                            pixelRatio: sldpr.value
                        }

                    }

                    LabelSpacer {
                    }

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

                    LabelSpacer {
                    }

                    MuotoButton {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: Math.min(parent.width - Theme.paddingLarge * 2, Theme.buttonWidthMedium)
                        text: qsTr("Restore default")
                        enabled: dlg.densityReady && dlg.vendorDprKnown && !dlg.dprAtDefault
                        onClicked: dlg.restoreDefaultDpr()
                    }

                    LabelSpacer {
                    }

                    MuotoTextLabel {
                        text: qsTr("Controls how large Sailfish UI elements appear. " + "Lower = more on screen; higher = larger text and buttons.")
                    }

                }

                Column {
                    width: isLandscape ? parent.width / 2 : parent.width

                    LabelSpacer {
                    }

                    ComboBox {
                        id: cbiz

                        width: parent.width
                        label: qsTr("Launcher icon size")
                        enabled: dlg.densityReady
                        description: qsTr("Icons on the home screen and app grid. " + "System default uses your device's normal size.")

                        menu: ContextMenu {
                            MenuItem {
                                text: qsTr("System default")
                            }

                            MenuItem {
                                text: qsTr("Compact (86)")
                            }

                            MenuItem {
                                text: qsTr("Normal (108)")
                            }

                            MenuItem {
                                text: qsTr("Medium (129)")
                            }

                            MenuItem {
                                text: qsTr("Large (151)")
                            }

                            MenuItem {
                                text: qsTr("Extra large (172)")
                            }

                        }

                    }

                    LabelSpacer {
                    }

                }

            }

            HomescreenRestartSection {
                id: restartSection

                settings: dlg.settings
                explanation: qsTr("Restart the homescreen after applying display density so all apps pick up the changes.")
            }

            LabelSpacer {
            }

        }

    }

}
