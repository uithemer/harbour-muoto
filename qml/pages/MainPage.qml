import Nemo.DBus 2.0
import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0
import "../components"

Page {
    id: mainpage

    ThemeWork {
        id: themeWork
        reloadActive: mainpage.status === PageStatus.Active
    }

    function refreshHomeIconPreview() {
        if (settings.isRunning)
            return
        iconapplier.buildPreview(settings.hasActiveIconPack()
                                 ? settings.activeIconPack : "default")
    }

    Component.onCompleted: refreshHomeIconPreview()

    onStatusChanged: {
        if (status === PageStatus.Active) {
            app.coverMode = "mainPage"
            refreshHomeIconPreview()
        }
    }

    Connections {
        target: settings
        onActiveIconPackChanged: mainpage.refreshHomeIconPreview()
        onIsRunningChanged: {
            if (!settings.isRunning)
                mainpage.refreshHomeIconPreview()
        }
    }

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentHeight: content.height
        enabled: !settings.isRunning
        opacity: settings.isRunning ? 0.2 : 1.0

        PullDownMenu {
            flickable: flickable
            enabled: !settings.isRunning

            MuotoAboutMenuItem { }

            MuotoRestartHomescreenMenuItem {
                remorsePopup: themeWork.remorsePopup
                themePack: themeWork.themePack
            }

            MenuItem {
                visible: themeWork.themePack.hasStoremanInstalled()
                text: qsTr("Download more themes")
                onClicked: openStore.call('openPage',
                    ['SearchPage', { initialSearch: 'themepack' }])
            }
        }

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width

            PageHeader {
                title: qsTr("Muoto")
            }

            Grid {
                width: parent.width
                columns: isLandscape ? 4 : 2
                spacing: Theme.paddingMedium

                HomeTile {
                    width: (parent.width - (parent.columns - 1) * parent.spacing)
                           / parent.columns
                    title: qsTr("Icons")
                    subtitle: settings.hasActiveIconPack()
                              ? themeWork.packLabel(settings.activeIconPack)
                              : qsTr("Stock")
                    onClicked: pageStack.push(Qt.resolvedUrl("IconsConfigurePage.qml"), {
                        "themeWork": themeWork,
                        "settings": settings
                    })

                    IconPackPreview {
                        width: parent.width
                        packName: settings.hasActiveIconPack()
                                    ? settings.activeIconPack : "default"
                        previewHeight: Math.min(width, Theme.itemSizeLarge * 1.5)
                    }
                }

                HomeTile {
                    width: (parent.width - (parent.columns - 1) * parent.spacing)
                           / parent.columns
                    title: qsTr("Fonts")
                    subtitle: settings.hasActiveFontPack()
                              ? themeWork.packLabel(settings.activeFontPack)
                              : qsTr("Stock")
                    onClicked: pageStack.push(Qt.resolvedUrl("FontsConfigurePage.qml"), {
                        "themeWork": themeWork,
                        "settings": settings
                    })

                    Label {
                        width: parent.width
                        height: Math.min(width, Theme.itemSizeLarge * 1.5)
                        text: "Aa"
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeHuge
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.WordWrap
                    }
                }

                HomeTile {
                    width: (parent.width - (parent.columns - 1) * parent.spacing)
                           / parent.columns
                    title: qsTr("Display density")
                    subtitle: qsTr("Tap to configure")
                    onClicked: pageStack.push(Qt.resolvedUrl("DensityPage.qml"), {
                        "themeWork": themeWork,
                        "settings": settings
                    })

                    Image {
                        anchors.centerIn: parent
                        anchors.bottomMargin: Theme.itemSizeLarge
                        width: Theme.iconSizeLarge
                        height: width
                        source: "image://theme/icon-m-size"
                    }
                }

                HomeTile {
                    width: (parent.width - (parent.columns - 1) * parent.spacing)
                           / parent.columns
                    title: qsTr("Dynamic icons")
                    subtitle: {
                        var ap = settings.hasActiveIconPack()
                                 ? settings.activeIconPack : "default"
                        var stock = !settings.hasActiveIconPack()
                        var clk = stock || themeWork.themepackmodel.hasDynClockForPack(ap)
                        var cal = stock || themeWork.themepackmodel.hasDynCalendarForPack(ap)
                        if (!clk && !cal)
                            return qsTr("Not available")
                        var on = (clk && settings.dynamicClockEnabled)
                               || (cal && settings.dynamicCalendarEnabled)
                        return on ? qsTr("On") : qsTr("Off")
                    }
                    onClicked: pageStack.push(Qt.resolvedUrl("DynamicIconsPage.qml"))

                    Image {
                        anchors.centerIn: parent
                        anchors.bottomMargin: Theme.itemSizeLarge
                        width: Theme.iconSizeLarge
                        height: width
                        source: "image://theme/icon-clock"
                    }
                }
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }
        }
    }

    DBusInterface {
        id: openStore
        service: 'harbour.storeman.service'
        path: '/harbour/storeman/service'
        iface: 'harbour.storeman.service'
    }

    BusyState { id: busyindicator }
}
