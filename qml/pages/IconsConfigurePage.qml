import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0
import "../common"
import "../common/fontWeightUtils.js" as FontWeightUtils
import "../components"

Dialog {
    id: dlg

    property var themeWork
    property Settings settings

    property int selectedIndex: -1
    property bool overlaySelected: false
    property bool dynClockSelected: false
    property bool dynCalendarSelected: false

    readonly property ThemePackModel packModel: themeWork.themepackmodel
    readonly property int effectiveIndex: {
        if (selectedIndex >= 0)
            return selectedIndex
        var idx = themeWork.indexForPackName(settings.activeIconPack)
        if (idx >= 0 && packModel.hasIcons(idx))
            return idx
        return themeWork.firstIconPackIndex()
    }
    readonly property string packName: effectiveIndex >= 0
                                     ? packModel.packName(effectiveIndex) : ""
    readonly property bool hasNative: effectiveIndex >= 0 && packModel.hasNative(effectiveIndex)
    readonly property bool hasJolla: effectiveIndex >= 0 && packModel.hasJolla(effectiveIndex)
    readonly property bool hasApk: effectiveIndex >= 0 && packModel.hasApk(effectiveIndex)
    readonly property bool hasIconApply: hasNative || hasApk || hasJolla
    readonly property bool hasIconOverlay: effectiveIndex >= 0
                                           && packModel.hasIconOverlay(effectiveIndex)
    readonly property bool hasDynClock: effectiveIndex >= 0
                                        && packModel.hasDynClock(effectiveIndex)
    readonly property bool hasDynCalendar: effectiveIndex >= 0
                                           && packModel.hasDynCalendar(effectiveIndex)
    readonly property bool wantsIconOps: hasIconApply || overlaySelected

    canAccept: effectiveIndex >= 0 && wantsIconOps

    ListModel { id: carouselModel }

    function rebuildCarousel() {
        carouselModel.clear()
        for (var i = 0; i < packModel.rowCount(); ++i) {
            if (!packModel.hasIcons(i))
                continue
            carouselModel.append({
                packIndex: i,
                packName: packModel.packName(i),
                packDisplayName: packModel.packDisplayName(i)
            })
        }
    }

    function initFromSettings() {
        overlaySelected = settings.iconOverlay
        dynClockSelected = settings.dynamicClockEnabled
        dynCalendarSelected = settings.dynamicCalendarEnabled
        selectedIndex = themeWork.indexForPackName(settings.activeIconPack)
        if (selectedIndex < 0 || !packModel.hasIcons(selectedIndex))
            selectedIndex = themeWork.firstIconPackIndex()
    }

    Component.onCompleted: {
        rebuildCarousel()
        initFromSettings()
    }

    onStatusChanged: {
        if (status === PageStatus.Active)
            rebuildCarousel()
    }

    Connections {
        target: packModel
        onModelReset: dlg.rebuildCarousel()
    }

    onEffectiveIndexChanged: {
        if (!hasIconOverlay)
            overlaySelected = false
        else if (themeWork.indexForPackName(settings.activeIconPack) === effectiveIndex)
            overlaySelected = settings.iconOverlay
        else
            overlaySelected = hasIconApply
    }

    onAccepted: {
        settings.homeRefresh = restartSection.homeRefreshSwitch.checked
        if (hasDynClock)
            settings.dynamicClockEnabled = dynClockSelected
        if (hasDynCalendar)
            settings.dynamicCalendarEnabled = dynCalendarSelected

        var runPack = hasIconApply
        if (overlaySelected && !runPack)
            runPack = true

        themeWork.applyIconsOnly(packName, runPack, overlaySelected)
    }

    BusyState { id: busyindicator }

    DialogHeader {
        id: header
        dialog: dlg
        cancelText: qsTr("Cancel")
        acceptText: qsTr("Apply")
    }

    SilicaFlickable {
        id: flickable
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        contentHeight: content.height
        enabled: !settings.isRunning
        opacity: settings.isRunning ? 0.2 : 1.0

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }


            IconPackPreview {
                width: parent.width
                packName: dlg.packName
                previewHeight: Math.min(parent.width, Math.max(320, flickable.height * 0.38))
            }

            SectionHeader { text: qsTr("Icon packs") }

            ListView {
                id: carousel
                width: parent.width
                height: Theme.itemSizeLarge * 1.6
                orientation: ListView.Horizontal
                spacing: Theme.paddingMedium
                clip: true
                model: carouselModel

                delegate: BackgroundItem {
                    width: carousel.height * 0.72
                    height: carousel.height
                    highlighted: model.packIndex === dlg.effectiveIndex

                    MouseArea {
                        anchors.fill: parent
                        onClicked: dlg.selectedIndex = model.packIndex
                    }

                    IconPackCarouselTile {
                        anchors.fill: parent
                        anchors.margins: Theme.paddingSmall
                        packName: model.packName
                        packDisplayName: model.packDisplayName
                    }
                }
            }

            SectionHeader {
                text: qsTr("Options")
                visible: hasIconOverlay || hasDynClock || hasDynCalendar
            }

            IconTextSwitch {
                automaticCheck: true
                text: qsTr("Style missing app icons")
                description: qsTr("Uses this theme's look for apps that don't have a custom icon in the pack.")
                visible: hasIconOverlay
                checked: overlaySelected
                enabled: hasIconOverlay
                onCheckedChanged: overlaySelected = checked
            }

            IconTextSwitch {
                automaticCheck: true
                text: qsTr("Dynamic clock icon")
                description: qsTr("Show the current time on the Clock icon, in this theme's style.")
                visible: hasDynClock
                checked: dynClockSelected
                onCheckedChanged: dynClockSelected = checked
            }

            IconTextSwitch {
                automaticCheck: true
                text: qsTr("Dynamic calendar icon")
                description: qsTr("Show today's date on the Calendar icon, in this theme's style.")
                visible: hasDynCalendar
                checked: dynCalendarSelected
                onCheckedChanged: dynCalendarSelected = checked
            }

            HomescreenRestartSection {
                id: restartSection
                settings: dlg.settings
                explanation: qsTr("Restart the homescreen after applying icons so all launcher tiles refresh.")
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }
        }
    }
}
