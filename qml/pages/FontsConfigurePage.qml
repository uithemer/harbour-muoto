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
    // Applied pack is already the process UI face via 99-muoto.conf. FontLoader
    // of those TTFs then unregister (preview reload) aborts Qt 5.6 fontconfig.
    readonly property bool previewUsesThemeFamily: stockSelected
        || (hasFont && packName !== "" && packName === settings.activeFontPack)

    canAccept: stockSelected || (effectiveIndex >= 0 && fontsApplyOk)

    ListModel { id: carouselModel }

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

    FontWeightModel {
        id: carouselProbe
        packName: ""
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

    function sampleBasenameForPack(packIndex) {
        if (!packModel.hasFont(packIndex))
            return ""
        carouselProbe.packName = packModel.packName(packIndex)
        var sample = FontWeightUtils.pickPreferredBasename(carouselProbe)
        carouselProbe.packName = ""
        return sample
    }

    function rebuildCarousel() {
        carouselModel.clear()
        carouselModel.append({
            packIndex: -1,
            packName: "",
            packDisplayName: qsTr("Default"),
            sampleFontBasename: "",
            isDefault: true
        })
        for (var i = 0; i < packModel.rowCount(); ++i) {
            if (!packModel.hasFont(i) && !packModel.hasFontNonLatin(i))
                continue
            carouselModel.append({
                packIndex: i,
                packName: packModel.packName(i),
                packDisplayName: packModel.packDisplayName(i),
                sampleFontBasename: sampleBasenameForPack(i),
                isDefault: false
            })
        }
        scheduleCenterCarousel()
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
        scheduleCenterCarousel()
    }

    function syncWeightForPack() {
        selectedFont = resolveSelectedWeight()
    }

    Component.onCompleted: {
        rebuildCarousel()
        syncFromSettings()
    }

    onStatusChanged: {
        if (status === PageStatus.Active)
            syncFromSettings()
    }

    Connections {
        target: packModel
        onModelReset: dlg.rebuildCarousel()
    }

    onEffectiveIndexChanged: {
        syncWeightForPack()
        scheduleCenterCarousel()
    }

    onPackNameChanged: schedulePreviewReload()
    onSelectedFontChanged: schedulePreviewReload()
    onStockSelectedChanged: schedulePreviewReload()

    function schedulePreviewReload() {
        previewLoader.source = ""
        previewReloadTimer.restart()
    }

    Timer {
        id: previewReloadTimer
        interval: 1
        onTriggered: {
            if (dlg.previewUsesThemeFamily)
                return
            if (!dlg.hasFont || dlg.packName === "" || dlg.selectedFont === "")
                return
            previewLoader.setSource(Qt.resolvedUrl("../components/FontPreview.qml"), {
                "packName": dlg.packName,
                "selectedFont": dlg.selectedFont
            })
        }
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
            
            Item {
                id: fontPreviewHost
                width: parent.width
                height: Math.min(parent.width, Math.max(280, flickable.height * 0.32))

                Loader {
                    id: previewLoader
                    anchors.fill: parent
                    visible: !dlg.previewUsesThemeFamily && hasFont && selectedFont !== ""
                }

                Column {
                    visible: dlg.previewUsesThemeFamily
                    anchors.fill: parent
                    anchors.margins: Theme.paddingLarge
                    spacing: Theme.paddingMedium

                    Label {
                        width: parent.width
                        font.pixelSize: Theme.fontSizeExtraLarge
                        font.weight: dlg.stockSelected
                                     ? Font.Light
                                     : FontWeightUtils.fontWeightFromBasename(dlg.selectedFont)
                        text: "Lorem ipsum"
                        wrapMode: Text.WordWrap
                    }

                    Label {
                        width: parent.width
                        font.pixelSize: Theme.fontSizeMedium
                        font.weight: dlg.stockSelected
                                     ? Font.Light
                                     : FontWeightUtils.fontWeightFromBasename(dlg.selectedFont)
                        text: "Dolor sit amet, consectetur adipiscing elit. Maecenas imperdiet finibus venenatis. Suspendisse mollis urna sed luctus sodales."
                        wrapMode: Text.WordWrap
                    }
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

                    FontPackCarouselTile {
                        anchors.fill: parent
                        anchors.margins: Theme.paddingSmall
                        packName: model.packName
                        packDisplayName: model.packDisplayName
                        sampleFontBasename: model.sampleFontBasename
                        loadOwnFont: !model.isDefault
                                     && model.packName !== settings.activeFontPack
                                     && model.packName !== dlg.packName
                                     && (dlg.previewUsesThemeFamily
                                         || previewLoader.source !== "")
                    }
                }
            }

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
}
