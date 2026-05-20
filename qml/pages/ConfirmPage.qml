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
    property string packDisplayName: themePackModel.packDisplayName(themePackIndex)
    property string packName: themePackModel.packName(themePackIndex)
    property alias iconsSelected: itsicons.checked
    property alias iconOverlaySelected: itsiconoverlay.checked
    property alias fontsSelected: itsfonts.checked
    property string selectedFont: ""
    property string confirmheadername: "%1".arg(packDisplayName)

    id: dlgconfirm
    focus: true
    canAccept: itsicons.checked || itsiconoverlay.checked || (itsfonts.checked && selectedFont !== "") || (itsfonts.checked && !hasFont && hasFontNonLatin)

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
            if (packName !== dlgconfirm.packName)
                return

            busyimg.running = false

            if (ok) {
                imgpreviewfallback.visible = false
                imgpreview.source = ""
                imgpreview.source = "image://uithemer/preview/" + packName + "?t=" + Date.now()
            } else {
                imgpreview.source = ""
                imgpreviewfallback.visible = true
            }
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
                visible: hasIcons || hasIconOverlay
            }

            Grid {
                width: parent.width
                visible: hasIcons || hasIconOverlay
                columns: isLandscape ? 2 : 1

            Column
            {
                width: isLandscape ? parent.width/2 : parent.width

            Item {
                id: iconpreview
                width: parent.width
                height: 450

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
                visible: hasIcons
                checked: hasIcons
                enabled: hasIcons
                onClicked: {
                    iconsSelected = itsicons.checked;
                    itsiconoverlay.checked = itsicons.checked;

                    if(!itsicons.checked && !itsfonts.checked)
                        dlgconfirm.canAccept = false
                    else
                        dlgconfirm.canAccept = true
                }
            }

            IconTextSwitch {
                id: itsiconoverlay
                automaticCheck: true
                text: qsTr("Apply icon overlay")
                description: qsTr("The theme supports overlays.")
                visible: hasIconOverlay
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
                    active: hasFont || hasFontNonLatin
                    source: ""
                    width: parent.width
                    height: 350
                    visible: false

                    function reload() {
                        source = ""
                        if (hasFont || hasFontNonLatin)
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

                    if(!itsicons.checked && !itsfonts.checked)
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
