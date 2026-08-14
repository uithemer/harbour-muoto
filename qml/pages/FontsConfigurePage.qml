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
    property string selectedFont: ""

    readonly property ThemePackModel packModel: themeWork.themepackmodel
    readonly property int effectiveIndex: {
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

    canAccept: effectiveIndex >= 0 && fontsApplyOk

    ListModel { id: carouselModel }

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
        for (var i = 0; i < packModel.rowCount(); ++i) {
            if (!packModel.hasFont(i) && !packModel.hasFontNonLatin(i))
                continue
            carouselModel.append({
                packIndex: i,
                packName: packModel.packName(i),
                packDisplayName: packModel.packDisplayName(i),
                sampleFontBasename: sampleBasenameForPack(i)
            })
        }
    }

    function syncFromSettings() {
        cacheActiveFontWeightFromConf()
        selectedIndex = themeWork.indexForPackName(settings.activeFontPack)
        if (selectedIndex < 0 || !(packModel.hasFont(selectedIndex)
                                   || packModel.hasFontNonLatin(selectedIndex)))
            selectedIndex = -1
        selectedFont = resolveSelectedWeight()
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

    onEffectiveIndexChanged: syncWeightForPack()

    onPackNameChanged: schedulePreviewReload()
    onSelectedFontChanged: schedulePreviewReload()

    function schedulePreviewReload() {
        previewReloadTimer.unloadFirst = true
        previewReloadTimer.restart()
    }

    Timer {
        id: previewReloadTimer
        interval: 1
        property bool unloadFirst: true
        onTriggered: {
            if (unloadFirst) {
                previewLoader.source = ""
                unloadFirst = false
                start()
                return
            }
            unloadFirst = true
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
                width: parent.width
                height: Math.min(parent.width, Math.max(280, flickable.height * 0.32))

                Loader {
                    id: previewLoader
                    anchors.fill: parent
                    visible: hasFont && selectedFont !== ""
                }

                Label {
                    anchors.centerIn: parent
                    width: parent.width - Theme.paddingLarge * 2
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    color: Theme.secondaryColor
                    visible: !hasFont && hasFontNonLatin
                    text: qsTr("This pack provides non-Latin fonts only.")
                }

                Label {
                    anchors.centerIn: parent
                    width: parent.width - Theme.paddingLarge * 2
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    color: Theme.secondaryColor
                    visible: hasFont && selectedFont === ""
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

                delegate: BackgroundItem {
                    width: carousel.height * 0.72
                    height: carousel.height
                    highlighted: model.packIndex === dlg.effectiveIndex

                    MouseArea {
                        anchors.fill: parent
                        onClicked: dlg.selectedIndex = model.packIndex
                    }

                    FontPackCarouselTile {
                        anchors.fill: parent
                        anchors.margins: Theme.paddingSmall
                        packName: model.packName
                        packDisplayName: model.packDisplayName
                        sampleFontBasename: model.sampleFontBasename
                    }
                }
            }

            SectionHeader {
                text: qsTr("Font weight")
                visible: hasFont
            }

            Repeater {
                id: weightRepeater
                model: fontweightmodel

                delegate: FontWeightSwitch {
                    width: parent.width
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
