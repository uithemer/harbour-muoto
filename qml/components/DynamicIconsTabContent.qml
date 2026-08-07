import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0
import "."

SilicaFlickable {
    id: dynView
    anchors.fill: parent
    contentHeight: dynContent.height
    enabled: !settings.isRunning && dynAvailable
    opacity: (!dynAvailable || settings.isRunning) ? 0.4 : 1.0

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

    Column {
        id: dynContent
        width: parent.width

        PageHeader {
            title: qsTr("Dynamic icons")
        }

        MuotoTextLabel {
            text: dynView.dynAvailable
                  ? (dynView.stockDyn
                     ? qsTr("Live clock and calendar on the launcher using stock icons.")
                     : qsTr("Live clock and calendar on the launcher using the active theme's assets."))
                  : qsTr("This theme has no dynamic clock or calendar assets. Apply a theme that includes them, or restore the default theme.")
        }

        Item {
            width: parent.width
            height: Theme.paddingLarge
            visible: dynView.hasActiveDynClock
        }

        Image {
            id: clockPreview
            anchors.horizontalCenter: parent.horizontalCenter
            width: Theme.iconSizeLauncher * 2
            height: width
            fillMode: Image.PreserveAspectFit
            sourceSize.width: width
            sourceSize.height: height
            visible: dynView.hasActiveDynClock
            opacity: settings.dynamicClockEnabled ? 1.0 : 0.4
            source: visible
                    ? ("image://muoto-launcher/dyn-clock/" + dynView.activePack
                       + "?t=" + dynView.previewTick)
                    : ""
        }

        IconTextSwitch {
            id: itsdynclock
            width: parent.width
            automaticCheck: true
            visible: dynView.hasActiveDynClock
            enabled: dynView.hasActiveDynClock && !settings.isRunning
            text: qsTr("Dynamic clock icon")
            description: qsTr("Shows the current time on the Clock launcher icon.")
            checked: dynView.hasActiveDynClock && settings.dynamicClockEnabled
            onClicked: {
                if (dynView.hasActiveDynClock)
                    settings.dynamicClockEnabled = itsdynclock.checked
            }
        }

        Item {
            width: parent.width
            height: Theme.paddingLarge
            visible: dynView.hasActiveDynCalendar
        }

        Image {
            id: calendarPreview
            anchors.horizontalCenter: parent.horizontalCenter
            width: Theme.iconSizeLauncher * 2
            height: width
            fillMode: Image.PreserveAspectFit
            sourceSize.width: width
            sourceSize.height: height
            visible: dynView.hasActiveDynCalendar
            opacity: settings.dynamicCalendarEnabled ? 1.0 : 0.4
            source: visible
                    ? ("image://muoto-launcher/dyn-calendar/" + dynView.activePack
                       + "?t=" + dynView.previewTick)
                    : ""
        }

        IconTextSwitch {
            id: itsdyncal
            width: parent.width
            automaticCheck: true
            visible: dynView.hasActiveDynCalendar
            enabled: dynView.hasActiveDynCalendar && !settings.isRunning
            text: qsTr("Dynamic calendar icon")
            description: qsTr("Shows today's date on the Calendar launcher icon.")
            checked: dynView.hasActiveDynCalendar && settings.dynamicCalendarEnabled
            onClicked: {
                if (dynView.hasActiveDynCalendar)
                    settings.dynamicCalendarEnabled = itsdyncal.checked
            }
        }

        Item {
            width: parent.width
            height: Theme.paddingLarge
        }
    }
}
