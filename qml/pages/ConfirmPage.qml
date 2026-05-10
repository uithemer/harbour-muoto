import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.uithemer 1.0
import "../common"
import "../components"

Dialog
{
    property Settings settings

    property ThemePackModel themePackModel
    property int themePackIndex

    property bool hasIcons: themePackModel.hasIcons(themePackIndex)
    property bool hasIconOverlay: themePackModel.hasIconOverlay(themePackIndex)
    property bool hasFont: themePackModel.hasFont(themePackIndex)
    property bool hasFontNonLatin: themePackModel.hasFontNonLatin(themePackIndex)
    property bool hasSound: themePackModel.hasSound(themePackIndex)
    property string packDisplayName: themePackModel.packDisplayName(themePackIndex)
    property string packName: themePackModel.packName(themePackIndex)
    property alias iconsSelected: itsicons.checked
    property alias iconOverlaySelected: itsiconoverlay.checked
    property alias fontsSelected: itsfonts.checked
    property alias soundsSelected: itssounds.checked
    property string selectedFont: ""
    property string confirmheadername: "%1".arg(packDisplayName)

    id: dlgconfirm
    focus: true
    canAccept: itsicons.checked || itsiconoverlay.checked || (itsfonts.checked && selectedFont !== "") || (itsfonts.checked && !hasFont && hasFontNonLatin) || itssounds.checked
    backNavigation: !settings.isRunning
    forwardNavigation: !settings.isRunning
    showNavigationIndicator: !settings.isRunning

    BusyState { id: busyindicator }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            app.coverMode = "confirmDialog"
        }
    }

    onAccepted: {
        settings.homeRefresh = tshomerefresh.checked;
    }

    Component.onCompleted: {
        if (hasIcons || hasIconOverlay)
                iconapplier.buildPreview(packName)
    }

    Connections {
        target: iconapplier
        onPreviewReady: {
            imgpreview.source = ""
            imgpreview.source = "/usr/share/sailfishos-uithemer/tmp/iconspreview.png"
            busyimg.running = false
        }
    }

    Keys.onPressed: {
        handleKeyPressed(event);
    }

    function handleKeyPressed(event) {

        if (event.key === Qt.Key_Down) {
            flickable.flick(0, - dlgconfirm.height);
            event.accepted = true;
        }

        if (event.key === Qt.Key_Up) {
            flickable.flick(0, dlgconfirm.height);
            event.accepted = true;
        }

        if (event.key === Qt.Key_PageDown) {
            flickable.scrollToBottom();
            event.accepted = true;
        }

        if (event.key === Qt.Key_PageUp) {
            flickable.scrollToTop();
            event.accepted = true;
        }

        if ((event.key === Qt.Key_B) || (event.key === Qt.Key_C)) {
            pageStack.navigateBack();
            event.accepted = true;
        }

        if (event.key === Qt.Key_Return) {
            dlgconfirm.accept();
            event.accepted = true;
        }

        if (event.key === Qt.Key_G) {
            pageStack.push(Qt.resolvedUrl("GuidePage.qml"));
            event.accepted = true;
        }
    }

    FontWeightModel { id: fontweightmodel; packName: dlgconfirm.packName }

    SilicaFlickable
    {
        id: flickable
        anchors.fill: parent
        contentHeight: content.height
        width: parent.width
        enabled: !settings.isRunning
        opacity: settings.isRunning ? 0.2 : 1.0

        VerticalScrollDecorator { }

        Column
        {
            id: content
            width: parent.width

            DialogHeader { id: header; cancelText: qsTr("Cancel"); acceptText: qsTr("Apply") }

            ConfirmHeader { text: dlgconfirm.confirmheadername }

            SectionHeader {
                text: qsTr("Icons")
                visible: hasIcons || hasIconOverlay
            }

            Grid {
                width: parent.width
                visible: hasIcons || hasIconOverlay
                columns: isLandscape ? 2 : 1

            Column
            {
                width: isLandscape ? parent.width/2 : parent.width

            Column {
                id: iconpreview
                width: parent.width
                height: 450
                BusyIndicator { id: busyimg; running: true; size: BusyIndicatorSize.Medium; anchors.centerIn: parent }
                Image {
                    id: imgpreview
                    height: 450
                    anchors.horizontalCenter: parent.horizontalCenter
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    cache: false
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
                visible: hasIcons
                checked: hasIcons
                enabled: hasIcons
                onClicked: {
                    iconsSelected = itsicons.checked;
                    itsiconoverlay.checked = itsicons.checked;

                    if(!itsicons.checked && !itsfonts.checked && !itssounds.checked)
                        dlgconfirm.canAccept = false
                    else
                        dlgconfirm.canAccept = true
                }
            }

            LabelText {
                visible: hasIconOverlay && settings.guimode === 0
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                text: qsTr("The theme supports overlays.")
            }

            IconTextSwitch {
                id: itsiconoverlay
                automaticCheck: true
                text: qsTr("Apply icon overlay")
                description: qsTr("The theme supports overlays.")
                visible: hasIconOverlay && settings.guimode !== 0
                checked: hasIconOverlay
                enabled: hasIconOverlay && itsicons.checked
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
                    source: "../components/FontPreview.qml"
                    width: parent.width
                    height: 350
                    visible: false

                    function reload() {
                        source = ""
                        source = "../components/FontPreview.qml"
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

                    if(!itsicons.checked && !itsfonts.checked && !itssounds.checked)
                        dlgconfirm.canAccept = false
                    else
                        dlgconfirm.canAccept = true
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

            SectionHeader {
                text: qsTr("Sounds")
                visible: hasSound
            }
            IconTextSwitch {
                id: itssounds
                automaticCheck: true
                text: qsTr("Apply sounds")
                visible: hasSound
                checked: hasSound
                enabled: hasSound
                onClicked: {
                    soundsSelected = itssounds.checked;

                    if(!itsicons.checked && !itsfonts.checked && !itssounds.checked)
                        dlgconfirm.canAccept = false
                    else
                        dlgconfirm.canAccept = true
                }
            }

                }

            } // grid

                LabelText {
                    visible: settings.guimode === 0 ? false : true
                    text: "<br>" + qsTr("Remember to restart the homescreen right after.")
                }

                TextSwitch { id: tshomerefresh
                    visible: settings.guimode === 0 ? false : true
                    text: qsTr("Restart homescreen")
                    checked: settings.homeRefresh
                }

                LabelText {
                    visible: settings.guimode === 0
                    text: "<br>" + qsTr("After confirming, your device will restart. Your currently opened apps will be closed.")
                }

                LabelText {
                    text: qsTr("For sounds, a full restart may be needed to apply your settings.")
                }

                Item {
                    width: parent.width
                    height: Theme.paddingLarge
                }
        }
    }
}
