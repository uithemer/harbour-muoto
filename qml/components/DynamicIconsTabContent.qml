import QtQuick 2.0
import Sailfish.Silica 1.0
import Opal.Delegates 1.0 as D
import harbour.muoto 1.0
import "."

SilicaFlickable {
    id: dynView
    anchors.fill: parent
    contentHeight: dynContent.height
    enabled: !settings.isRunning
    opacity: settings.isRunning ? 0.4 : 1.0

    property bool tabActive: true
    property string previewTick: "0"

    readonly property string activePack: settings.hasActiveIconPack()
                                         ? settings.activeIconPack : "default"
    readonly property bool stockDyn: !settings.hasActiveIconPack()
    readonly property bool hasActiveDynClock: stockDyn
                                              || themepackmodel.hasDynClockForPack(activePack)
    readonly property bool hasActiveDynCalendar: stockDyn
                                                 || themepackmodel.hasDynCalendarForPack(activePack)
    readonly property bool dynAvailable: hasActiveDynClock || hasActiveDynCalendar

    ThemePackModel { id: themepackmodel }

    function bumpPreview() {
        previewTick = String(Date.now())
    }

    onTabActiveChanged: {
        if (tabActive)
            bumpPreview()
    }

    onActivePackChanged: bumpPreview()
    onDynAvailableChanged: bumpPreview()

    Component.onCompleted: bumpPreview()

    Timer {
        interval: 60 * 1000
        running: dynView.tabActive && dynView.dynAvailable && Qt.application.active
        repeat: true
        onTriggered: dynView.bumpPreview()
    }

    VerticalScrollDecorator { }

    PullDownMenu {
        flickable: dynView
        enabled: tabActive && !settings.isRunning

        MuotoAboutMenuItem { }
    }

    Column {
        id: dynContent
        width: parent.width
        spacing: Theme.paddingLarge

        PageHeader {
            title: qsTr("Dynamic icons")
        }

        MuotoTextLabel {
            // Keep intro readable even when no dyn assets (tab still scrollable).
            opacity: dynView.dynAvailable ? 1.0 : Theme.opacityHigh
            text: dynView.dynAvailable
                  ? (dynView.stockDyn
                     ? qsTr("Show the current time and date on your Clock and Calendar icons.")
                     : qsTr("Show the current time and date on your Clock and Calendar icons, using this theme's style."))
                  : qsTr("This theme doesn't include live Clock or Calendar icons. Try another theme, or restore the default look.")
        }

        D.TwoLineDelegate {
            id: clockDelegate
            width: parent.width
            visible: dynView.hasActiveDynClock
            enabled: dynView.hasActiveDynClock && !settings.isRunning
            interactive: false
            minContentHeight: Theme.itemSizeLarge
            text: qsTr("Dynamic clock icon")
            description: qsTr("Shows the current time on the Clock launcher icon.")
            padding.topBottom: Theme.paddingMedium

            Component.onCompleted: descriptionLabel.wrapped = true

            leftItem: Component {
                TextSwitch {
                    id: clockSwitch
                    checked: dynView.hasActiveDynClock && settings.dynamicClockEnabled
                    enabled: dynView.hasActiveDynClock && !settings.isRunning
                    width: Theme.itemSizeSmall
                    height: Theme.itemSizeSmall
                    leftMargin: 0
                    rightMargin: 0
                    onCheckedChanged: {
                        if (!enabled)
                            return
                        if (checked !== settings.dynamicClockEnabled)
                            settings.dynamicClockEnabled = checked
                    }
                }
            }

            rightItem: Component {
                Image {
                    width: Theme.iconSizeLauncher
                    height: width
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: width
                    sourceSize.height: height
                    source: "image://muoto-launcher/dyn-clock/" + dynView.activePack
                            + "?t=" + dynView.previewTick
                }
            }
        }

        D.TwoLineDelegate {
            id: calendarDelegate
            width: parent.width
            visible: dynView.hasActiveDynCalendar
            enabled: dynView.hasActiveDynCalendar && !settings.isRunning
            interactive: false
            minContentHeight: Theme.itemSizeLarge
            text: qsTr("Dynamic calendar icon")
            description: qsTr("Shows today's date on the Calendar launcher icon.")
            padding.topBottom: Theme.paddingMedium

            Component.onCompleted: descriptionLabel.wrapped = true

            leftItem: Component {
                TextSwitch {
                    id: calendarSwitch
                    checked: dynView.hasActiveDynCalendar && settings.dynamicCalendarEnabled
                    enabled: dynView.hasActiveDynCalendar && !settings.isRunning
                    width: Theme.itemSizeSmall
                    height: Theme.itemSizeSmall
                    leftMargin: 0
                    rightMargin: 0
                    onCheckedChanged: {
                        if (!enabled)
                            return
                        if (checked !== settings.dynamicCalendarEnabled)
                            settings.dynamicCalendarEnabled = checked
                    }
                }
            }

            rightItem: Component {
                Image {
                    width: Theme.iconSizeLauncher
                    height: width
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: width
                    sourceSize.height: height
                    source: "image://muoto-launcher/dyn-calendar/" + dynView.activePack
                            + "?t=" + dynView.previewTick
                }
            }
        }

        Item {
            width: parent.width
            height: Theme.paddingLarge
        }
    }
}
