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
    property bool stockSelected: false
    property bool overlaySelected: false
    property bool dynClockSelected: false
    property bool dynCalendarSelected: false

    property bool appliedStock: false
    property int appliedIndex: -1
    property bool appliedOverlay: false
    property bool appliedDynClock: false
    property bool appliedDynCalendar: false

    readonly property ThemePackModel packModel: themeWork.themepackmodel
    readonly property int effectiveIndex: {
        if (stockSelected)
            return -1
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
    readonly property bool hasDynClock: stockSelected
                                        || (effectiveIndex >= 0
                                            && packModel.hasDynClock(effectiveIndex))
    readonly property bool hasDynCalendar: stockSelected
                                           || (effectiveIndex >= 0
                                               && packModel.hasDynCalendar(effectiveIndex))
    readonly property bool wantsIconOps: hasIconApply || overlaySelected

    readonly property bool dirty: {
        if (stockSelected !== appliedStock)
            return true
        if (stockSelected) {
            if (hasDynClock && dynClockSelected !== appliedDynClock)
                return true
            if (hasDynCalendar && dynCalendarSelected !== appliedDynCalendar)
                return true
            return false
        }
        if (effectiveIndex !== appliedIndex)
            return true
        if (hasIconOverlay && overlaySelected !== appliedOverlay)
            return true
        if (hasDynClock && dynClockSelected !== appliedDynClock)
            return true
        if (hasDynCalendar && dynCalendarSelected !== appliedDynCalendar)
            return true
        return false
    }

    canAccept: dirty && (stockSelected || (effectiveIndex >= 0 && wantsIconOps))

    ListModel { id: carouselModel }

    StockLauncherIcons { id: stockIcons }

    function carouselRowForPack(packIndex) {
        for (var i = 0; i < carouselModel.count; ++i) {
            if (carouselModel.get(i).packIndex === packIndex)
                return i
        }
        return -1
    }

    function centerCarouselNow() {
        if (!carousel || carousel.width <= 0 || carouselModel.count === 0)
            return false
        var row = carouselRowForPack(effectiveIndex)
        if (row < 0)
            return false
        carousel.currentIndex = row
        carousel.positionViewAtIndex(row, ListView.Contain)
        return true
    }

    function scheduleCenterCarousel() {
        centerCarouselTimer.tries = 0
        centerCarouselTimer.restart()
    }

    Timer {
        id: centerCarouselTimer
        interval: 50
        repeat: true
        property int tries: 0
        onTriggered: {
            tries += 1
            if (dlg.centerCarouselNow() || tries > 20)
                stop()
        }
    }

    function rebuildCarousel() {
        carouselModel.clear()
        carouselModel.append({
            packIndex: -1,
            packName: "",
            packDisplayName: qsTr("Default"),
            hasNative: false,
            hasApk: false,
            hasJolla: false,
            isDefault: true
        })
        for (var i = 0; i < packModel.rowCount(); ++i) {
            if (!packModel.hasIcons(i))
                continue
            carouselModel.append({
                packIndex: i,
                packName: packModel.packName(i),
                packDisplayName: packModel.packDisplayName(i),
                hasNative: packModel.hasNative(i),
                hasApk: packModel.hasApk(i),
                hasJolla: packModel.hasJolla(i),
                isDefault: false
            })
        }
        scheduleCenterCarousel()
    }

    function initFromSettings() {
        overlaySelected = settings.iconOverlay
        dynClockSelected = settings.dynamicClockEnabled
        dynCalendarSelected = settings.dynamicCalendarEnabled
        stockSelected = !settings.hasActiveIconPack()
        if (stockSelected) {
            selectedIndex = -1
        } else {
            selectedIndex = themeWork.indexForPackName(settings.activeIconPack)
            if (selectedIndex < 0 || !packModel.hasIcons(selectedIndex))
                selectedIndex = themeWork.firstIconPackIndex()
        }
        appliedStock = stockSelected
        appliedIndex = stockSelected ? -1 : selectedIndex
        appliedOverlay = overlaySelected
        appliedDynClock = dynClockSelected
        appliedDynCalendar = dynCalendarSelected
        scheduleCenterCarousel()
    }

    Component.onCompleted: {
        rebuildCarousel()
        initFromSettings()
    }

    onStatusChanged: {
        if (status === PageStatus.Active)
            scheduleCenterCarousel()
    }

    Connections {
        target: packModel
        onModelReset: dlg.rebuildCarousel()
    }

    onEffectiveIndexChanged: {
        if (!stockSelected) {
            if (!hasIconOverlay)
                overlaySelected = false
            else if (themeWork.indexForPackName(settings.activeIconPack) === effectiveIndex)
                overlaySelected = settings.iconOverlay
            else
                overlaySelected = hasIconApply
        }
        scheduleCenterCarousel()
    }

    onAccepted: {
        settings.homeRefresh = restartSection.homeRefreshSwitch.checked
        if (hasDynClock)
            settings.dynamicClockEnabled = itsdynclock.checked
        if (hasDynCalendar)
            settings.dynamicCalendarEnabled = itsdyncal.checked

        if (stockSelected) {
            themeWork.beginRestore(true, false)
            return
        }

        var runPack = hasIconApply
        if (overlaySelected && !runPack)
            runPack = true

        themeWork.applyIconsOnly(packName, runPack, overlaySelected)
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

            IconPackPreview {
                width: parent.width
                packName: dlg.stockSelected ? "default" : dlg.packName
                previewHeight: Math.min(parent.width, Math.max(280, flickable.height * 0.32))
                previewMargins: Theme.paddingLarge * 2
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
                boundsBehavior: Flickable.StopAtBounds

                delegate: BackgroundItem {
                    width: carousel.height * 0.72
                    height: carousel.height
                    highlighted: model.isDefault
                                 ? dlg.stockSelected
                                 : (!dlg.stockSelected && model.packIndex === dlg.effectiveIndex)

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (model.isDefault) {
                                dlg.stockSelected = true
                            } else {
                                dlg.stockSelected = false
                                dlg.selectedIndex = model.packIndex
                            }
                        }
                    }

                    IconPackCarouselTile {
                        anchors.fill: parent
                        anchors.margins: Theme.paddingSmall
                        packName: model.packName
                        packDisplayName: model.packDisplayName
                        hasNative: model.hasNative
                        hasApk: model.hasApk
                        hasJolla: model.hasJolla
                        isDefault: model.isDefault
                        stockModel: model.isDefault ? stockIcons : null
                    }
                }
            }

            LabelSpacer { }

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
                id: itsdynclock
                automaticCheck: false
                text: qsTr("Dynamic clock icon")
                description: qsTr("Show the current time on the Clock icon, in this theme's style.")
                visible: hasDynClock
                checked: dynClockSelected
                onClicked: dynClockSelected = !dynClockSelected
            }

            IconTextSwitch {
                id: itsdyncal
                automaticCheck: false
                text: qsTr("Dynamic calendar icon")
                description: qsTr("Show today's date on the Calendar icon, in this theme's style.")
                visible: hasDynCalendar
                checked: dynCalendarSelected
                onClicked: dynCalendarSelected = !dynCalendarSelected
            }

            LabelSpacer { }
            
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
