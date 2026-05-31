import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0
import "../common"
import "../common/fontWeightUtils.js" as FontWeightUtils
import "../components"

Dialog
{
    property Settings settings

    property ThemePackModel themePackModel
    property int themePackIndex

    property bool hasNative: themePackModel.hasNative(themePackIndex)
    property bool hasJolla: themePackModel.hasJolla(themePackIndex)
    property bool hasApk: themePackModel.hasApk(themePackIndex)
    property bool hasIcons: themePackModel.hasIcons(themePackIndex)
    property bool hasIconApply: hasNative || hasApk || hasJolla
    property bool hasIconOverlay: themePackModel.hasIconOverlay(themePackIndex)
    property bool hasFont: themePackModel.hasFont(themePackIndex)
    property bool hasFontNonLatin: themePackModel.hasFontNonLatin(themePackIndex)
    property string packDisplayName: themePackModel.packDisplayName(themePackIndex)
    property string packName: themePackModel.packName(themePackIndex)
    property alias iconsSelected: itsicons.checked
    property alias iconOverlaySelected: itsiconoverlay.checked
    property alias fontsSelected: itsfonts.checked
    property string selectedFont: ""
    property string confirmheadername: "%1".arg(packDisplayName)
    property bool wantsIconOps: itsicons.checked || itsiconoverlay.checked
    property string _previewBuiltPack: ""
    property string _previewLoadedPack: ""
    readonly property bool iconPreviewAvailable: _previewLoadedPack !== ""
                                         && _previewLoadedPack === packName
    id: dlgconfirm
    canAccept: wantsIconOps
        || (itsfonts.checked && hasFont && selectedFont !== "")
        || (itsfonts.checked && !hasFont && hasFontNonLatin)

    BusyState { id: busyindicator }

    function refreshIconPreview() {
        if (!hasIcons || packName === "") {
            busyimg.running = false
            return
        }
        if (packName === _previewBuiltPack)
            return

        _previewBuiltPack = packName
        _previewLoadedPack = ""
        busyimg.running = true
        imgpreviewfallback.visible = false
        iconapplier.buildPreview(packName)
    }

    onStatusChanged: {
        if (status === PageStatus.Active)
            refreshIconPreview()
    }

    onAccepted: {
        settings.homeRefresh = restartSection.homeRefreshSwitch.checked;
    }

    Connections {
        target: iconapplier
        onPreviewReady: {
            if (packName !== dlgconfirm.packName)
                return

            busyimg.running = false

            if (ok) {
                if (_previewLoadedPack === dlgconfirm.packName)
                    return
                _previewLoadedPack = dlgconfirm.packName
                imgpreviewfallback.visible = false
                imgpreview.source = "image://muoto/preview/" + dlgconfirm.packName
                                    + "?t=" + Date.now()
            } else {
                _previewLoadedPack = ""
                imgpreview.source = ""
                imgpreviewfallback.visible = true
            }
        }
    }

    FontWeightModel { id: fontweightmodel; packName: dlgconfirm.packName }

    // Cover / in-page preview basename: explicit pick, else first file in pack /font/.
    readonly property string previewFontBasename: {
        if (selectedFont !== "")
            return selectedFont
        return FontWeightUtils.fontBasenameFromFilename(fontweightmodel.firstWeight)
    }

    readonly property bool hasLatinFontFiles: fontweightmodel.rowCount() > 0

    property string _coverFontNotifyKey: ""

    function notifyCoverFontPreviewReady() {
        if (previewFontBasename === "" || app.coverMode !== "confirmDialog")
            return
        var key = packName + "\0" + previewFontBasename
        if (_coverFontNotifyKey === key)
            return
        _coverFontNotifyKey = key
        app.coverFontPreviewSeq++
    }

    onPackNameChanged: _coverFontNotifyKey = ""

    Connections {
        target: fontweightmodel
        onFirstWeightChanged: notifyCoverFontPreviewReady()
    }

    Connections {
        target: Qt.application
        onStateChanged: {
            // Swipe to cover: re-notify so CoverConfirm picks up fonts even if seq
            // was bumped before the cover Connections existed (first confirm open).
            if (state !== Qt.ApplicationActive && app.coverMode === "confirmDialog") {
                _coverFontNotifyKey = ""
                notifyCoverFontPreviewReady()
            }
        }
    }

    Component.onCompleted: notifyCoverFontPreviewReady()

    DialogHeader {
        id: header
        dialog: dlgconfirm
        cancelText: qsTr("Cancel")
        acceptText: qsTr("Apply")
    }

    SilicaFlickable
    {
        id: flickable
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        contentHeight: content.height
        enabled: !settings.isRunning
        opacity: settings.isRunning ? 0.2 : 1.0

        VerticalScrollDecorator { }

        Column
        {
            id: content
            width: parent.width

            MuotoHeaderLabel { text: dlgconfirm.confirmheadername }

            SectionHeader {
                text: qsTr("Icons")
                visible: hasIcons
            }

            Grid {
                width: parent.width
                visible: hasIcons
                columns: isLandscape ? 2 : 1

            Column
            {
                width: isLandscape ? parent.width/2 : parent.width

            Item {
                id: iconpreview
                width: parent.width
                height: Math.min(parent.width,
                                Math.max(450, flickable.height * 0.40))

                BusyIndicator {
                    id: busyimg
                    running: true
                    size: BusyIndicatorSize.Medium
                    anchors.centerIn: parent
                }

                Image {
                    id: imgpreview
                    anchors.fill: parent
                    anchors.margins: Theme.paddingMedium
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    cache: false
                    visible: status === Image.Ready
                    onStatusChanged: {
                        if (status === Image.Ready || status === Image.Error)
                            busyimg.running = false
                    }
                }


            LabelSpacer { }

                MuotoTextLabel {
                    id: imgpreviewfallback
                    visible: false
                    anchors.centerIn: parent
                    color: Theme.secondaryColor
                    text: qsTr("No preview available")
                }
            }

            }

            Column
            {
                width: isLandscape ? parent.width/2 : parent.width


                Item {
                    width: parent.width
                    height: Theme.paddingLarge
                }

            IconTextSwitch {
                id: itsicons
                automaticCheck: true
                text: qsTr("Apply icons")
                visible: hasIconApply
                checked: hasIconApply
                enabled: hasIconApply
                onClicked: {
                    iconsSelected = itsicons.checked;
                }
            }

            IconTextSwitch {
                id: itsiconoverlay
                automaticCheck: true
                text: qsTr("Style missing app icons")
                description: qsTr("Uses this theme's look for apps that don't have a custom icon in the pack.")
                visible: hasIconOverlay
                checked: hasIconOverlay
                enabled: hasIconOverlay
                onClicked: {
                    iconOverlaySelected = itsiconoverlay.checked;
                }
            }
            }
            } // grid

            SectionHeader {
                text: qsTr("Fonts")
                visible: hasFont || hasFontNonLatin
            }

            Grid {
                width: parent.width
                visible: hasFont || hasFontNonLatin
                columns: isLandscape ? 2 : 1

                Column
                {
                    width: isLandscape ? parent.width/2 : parent.width

                Loader {
                    id: fontloader
                    active: hasFont
                    source: ""
                    width: parent.width
                    height: 350
                    visible: false

                    function reload() {
                        source = ""
                        if (!hasFont || packName === "" || selectedFont === "")
                            return
                        setSource("../components/FontPreview.qml", {
                            "packName": packName,
                            "selectedFont": selectedFont
                        })
                    }
                }


            LabelSpacer { }

                MuotoTextLabel {
                    id: vphfont
                    width: parent.width - (x * 2)
                    height: 350
                    x: Theme.paddingLarge
                    text: qsTr("Choose a font weight to preview")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    truncationMode: TruncationMode.Fade
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeLarge
                    visible: hasFont
                }

                Item {
                    width: parent.width
                    height: Theme.paddingLarge
                }

                }

                Column
                {
                    width: isLandscape ? parent.width/2 : parent.width

            IconTextSwitch {
                id: itsfonts
                automaticCheck: true
                text: qsTr("Apply fonts")
                visible: hasFont || hasFontNonLatin
                checked: hasFont || hasFontNonLatin
                enabled: hasFont || hasFontNonLatin

                onClicked: {
                    fontsSelected = itsfonts.checked;
                }
            }

            Column {
                id: fontsettings
                width: parent.width
                visible: hasFont

                SectionHeader { text: qsTr("Font weight") }

                MuotoTextLabel {
                    text: qsTr("Choose the main font weight for the UI.")
                }

                Repeater {
                    id: views
                    model: fontweightmodel

                    delegate: FontWeightSwitch {
                        automaticCheck: true
                        enabled: itsfonts.checked
                        packName: dlgconfirm.packName
                        fontWeight: model.fontWeight
                        text: model.fontDisplayWeight

                        onClicked: {
                            var count = views.count;

                            for(var i = 0; i < views.count; i++)
                                views.itemAt(i).checked = false;

                            checked = true;
                            selectedFont = model.fontWeight;
                            vphfont.visible = false
                            fontloader.visible = true
                            fontloader.reload()
                            notifyCoverFontPreviewReady()
                        }
                    }
                }
            }

                }



            } // grid

                HomescreenRestartSection {
                    id: restartSection
                    settings: dlgconfirm.settings
                    explanation: qsTr("Restart the homescreen to refresh launcher icons and fonts and finish applying your theme.")
                }

                Item {
                    width: parent.width
                    height: Theme.paddingLarge
                }
        }
    }
}
