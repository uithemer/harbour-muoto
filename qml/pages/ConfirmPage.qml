import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0
import "../common"
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
        settings.homeRefresh = tshomerefresh.checked;
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

            ConfirmHeader { text: dlgconfirm.confirmheadername }

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
                                Math.max(450, flickable.height * 0.45))

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

                Label {
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
                text: qsTr("Apply icon overlay")
                description: qsTr("The theme supports overlays.")
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

                Label {
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

                LabelText {
                    text: qsTr("Choose the main font weight for the UI.")
                }

                Repeater {
                    id: views
                    model: fontweightmodel

                    delegate: IconTextSwitch {
                        automaticCheck: true
                        enabled: itsfonts.checked
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
                        }
                    }
                }
            }

                }

            } // grid

                LabelText {
                    text: "<br>" + qsTr("Launcher icons refresh automatically when you apply. Enable below only if icons stay stale (full lipstick restart).")
                }

                TextSwitch { id: tshomerefresh
                    text: qsTr("Restart homescreen (fallback)")
                    checked: settings.homeRefresh
                }

                Item {
                    width: parent.width
                    height: Theme.paddingLarge
                }
        }
    }
}
