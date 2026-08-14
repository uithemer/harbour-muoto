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
    property string selectedFont: ""
    property bool appliedStock: false
    property int appliedIndex: -1
    property string appliedWeight: ""

    readonly property ThemePackModel packModel: themeWork.themepackmodel
    readonly property int effectiveIndex: {
        if (stockSelected)
            return -1
        if (selectedIndex >= 0)
            return selectedIndex
        var idx = themeWork.indexForPackName(settings.activeFontPack)
        if (idx >= 0 && (packModel.hasFont(idx) || packModel.hasFontNonLatin(idx)))
            return idx
        return themeWork.firstFontPackIndex()
    }
    readonly property string packName: effectiveIndex >= 0
                                     ? packModel.packName(effectiveIndex) : ""
    readonly property bool hasFont: effectiveIndex >= 0 && packModel.hasFont(effectiveIndex)
    readonly property bool hasFontNonLatin: effectiveIndex >= 0
                                            && packModel.hasFontNonLatin(effectiveIndex)
    readonly property bool fontsApplyOk: !hasFont
        || (hasFont && selectedFont !== "")
        || (!hasFont && hasFontNonLatin)

    readonly property bool dirty: {
        if (stockSelected !== appliedStock)
            return true
        if (stockSelected)
            return false
        if (effectiveIndex !== appliedIndex)
            return true
        if (hasFont && selectedFont !== appliedWeight)
            return true
        return false
    }

    canAccept: dirty && (stockSelected || (effectiveIndex >= 0 && fontsApplyOk))

    FontCarouselModel {
        id: fontCarousel
        packModel: dlg.packModel
    }

    function centerCarouselNow() {
        if (!carousel || carousel.width <= 0)
            return false
        var row = fontCarousel.rowForPackIndex(effectiveIndex)
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

    FontWeightModel {
        id: fontweightmodel
        packName: dlg.hasFont ? dlg.packName : ""
    }

    function appliedWeightBasename() {
        if (settings.activeFontWeight && settings.activeFontWeight !== "")
            return settings.activeFontWeight
        return FontWeightUtils.activeWeightFromMuotoConf()
    }

    function cacheActiveFontWeightFromConf() {
        if (settings.activeFontWeight !== "" || !settings.hasActiveFontPack())
            return
        var w = FontWeightUtils.activeWeightFromMuotoConf()
        if (w !== "")
            settings.activeFontWeight = w
    }

    function resolveSelectedWeight() {
        if (!hasFont || fontweightmodel.rowCount() === 0)
            return ""

        if (settings.hasActiveFontPack()
                && themeWork.indexForPackName(settings.activeFontPack) === effectiveIndex) {
            var applied = appliedWeightBasename()
            if (FontWeightUtils.basenameExistsInModel(fontweightmodel, applied))
                return applied
        }

        return FontWeightUtils.pickPreferredBasename(fontweightmodel)
    }

    function syncFromSettings() {
        cacheActiveFontWeightFromConf()
        stockSelected = !settings.hasActiveFontPack()
        if (stockSelected) {
            selectedIndex = -1
        } else {
            selectedIndex = themeWork.indexForPackName(settings.activeFontPack)
            if (selectedIndex < 0 || !(packModel.hasFont(selectedIndex)
                                       || packModel.hasFontNonLatin(selectedIndex)))
                selectedIndex = -1
        }
        selectedFont = resolveSelectedWeight()
        appliedStock = stockSelected
        appliedIndex = stockSelected ? -1 : selectedIndex
        appliedWeight = selectedFont
        scheduleCenterCarousel()
    }

    function syncWeightForPack() {
        selectedFont = resolveSelectedWeight()
    }

    Component.onCompleted: syncFromSettings()

    onStatusChanged: {
        if (status === PageStatus.Active)
            syncFromSettings()
    }

    onEffectiveIndexChanged: {
        syncWeightForPack()
        scheduleCenterCarousel()
    }

    onAccepted: {
        settings.homeRefresh = restartSection.homeRefreshSwitch.checked
        if (stockSelected) {
            themeWork.beginRestore(false, true)
            return
        }
        themeWork.applyFontOnly(effectiveIndex, selectedFont, packName)
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

            Item {
                id: fontPreviewHost
                width: parent.width
                height: Math.min(parent.width, Math.max(280, flickable.height * 0.32))

                FontPreview {
                    anchors.fill: parent
                    visible: dlg.stockSelected
                             || (hasFont && selectedFont !== "")
                    packName: dlg.stockSelected ? "default" : dlg.packName
                    selectedFont: dlg.stockSelected ? "Light" : dlg.selectedFont
                }

                Label {
                    anchors.centerIn: parent
                    width: parent.width - Theme.paddingLarge * 2
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    color: Theme.secondaryColor
                    visible: !dlg.stockSelected && !hasFont && hasFontNonLatin
                    text: qsTr("This pack provides non-Latin fonts only.")
                }

                Label {
                    anchors.centerIn: parent
                    width: parent.width - Theme.paddingLarge * 2
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    color: Theme.secondaryColor
                    visible: !dlg.stockSelected && hasFont && selectedFont === ""
                    text: qsTr("Choose a font weight to preview")
                }
            }

            SectionHeader { text: qsTr("Font packs") }

            ListView {
                id: carousel
                width: parent.width
                height: Theme.itemSizeLarge * 1.6
                orientation: ListView.Horizontal
                spacing: Theme.paddingMedium
                clip: true
                model: fontCarousel
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

                    FontPackCarouselTile {
                        anchors.fill: parent
                        anchors.margins: Theme.paddingSmall
                        packName: model.packName
                        packDisplayName: model.packDisplayName
                        sampleFontBasename: model.sampleFontBasename
                        isDefault: model.isDefault
                    }
                }
            }

            LabelSpacer { }

            SectionHeader {
                text: qsTr("Font weight")
                visible: !stockSelected && hasFont
            }

            Repeater {
                id: weightRepeater
                model: fontweightmodel

                delegate: FontWeightSwitch {
                    width: parent.width
                    visible: !dlg.stockSelected
                    automaticCheck: false
                    enabled: hasFont
                    packName: dlg.packName
                    fontWeight: model.fontWeight
                    text: model.fontDisplayWeight
                    checked: dlg.selectedFont === model.fontWeight
                    onClicked: dlg.selectedFont = model.fontWeight
                }
            }

            LabelSpacer { }
            
            HomescreenRestartSection {
                id: restartSection
                settings: dlg.settings
                explanation: qsTr("Restart the homescreen after applying fonts so all apps pick up the new typeface.")
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }
        }
    }

    Connections {
        target: fontweightmodel
        onFirstWeightChanged: {
            if (!hasFont)
                return
            if (selectedFont !== ""
                    && FontWeightUtils.basenameExistsInModel(fontweightmodel, selectedFont))
                return
            selectedFont = resolveSelectedWeight()
        }
    }

    Connections {
        target: fontCarousel
        onModelReset: dlg.scheduleCenterCarousel()
    }
}
