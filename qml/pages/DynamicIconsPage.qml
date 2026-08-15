import QtQuick 2.0
import Sailfish.Silica 1.0
import "../common"
import "../components"

Dialog {
    id: dlg

    property var themeWork
    property Settings settings

    property string previewTick: "0"
    property bool clockSelected: false
    property bool calendarSelected: false
    property bool appliedClock: false
    property bool appliedCalendar: false

    readonly property string activePack: settings.hasActiveIconPack()
                                         ? settings.activeIconPack : "default"
    readonly property bool stockDyn: !settings.hasActiveIconPack()
    readonly property bool hasActiveDynClock: stockDyn
                                              || themeWork.themepackmodel.hasDynClockForPack(activePack)
    readonly property bool hasActiveDynCalendar: stockDyn
                                                 || themeWork.themepackmodel.hasDynCalendarForPack(activePack)
    readonly property bool dynAvailable: hasActiveDynClock || hasActiveDynCalendar
    readonly property bool dirty: (hasActiveDynClock && clockSelected !== appliedClock)
                                  || (hasActiveDynCalendar && calendarSelected !== appliedCalendar)

    canAccept: dynAvailable && dirty

    function bumpPreview() {
        previewTick = String(Date.now())
    }

    function initFromSettings() {
        clockSelected = hasActiveDynClock && settings.dynamicClockEnabled
        calendarSelected = hasActiveDynCalendar && settings.dynamicCalendarEnabled
        appliedClock = clockSelected
        appliedCalendar = calendarSelected
        bumpPreview()
    }

    Component.onCompleted: initFromSettings()

    onStatusChanged: {
        if (status === PageStatus.Active)
            bumpPreview()
    }

    Timer {
        interval: 60 * 1000
        running: dlg.status === PageStatus.Active
                 && ((dlg.hasActiveDynClock && dlg.clockSelected)
                     || (dlg.hasActiveDynCalendar && dlg.calendarSelected))
                 && Qt.application.active
        repeat: true
        onTriggered: dlg.bumpPreview()
    }

    onAccepted: {
        if (hasActiveDynClock)
            settings.dynamicClockEnabled = clockSelected
        if (hasActiveDynCalendar)
            settings.dynamicCalendarEnabled = calendarSelected
        app.showToast(qsTr("Settings applied."))
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

            Grid {
                width: parent.width
                columns: isLandscape ? 2 : 1

                Column {
                    width: isLandscape ? parent.width / 2 : parent.width

                    Item {
                        id: dynPreviewHost
                        width: parent.width
                        height: Math.min(parent.width, Math.max(280, flickable.height * 0.32))

                        Row {
                            anchors.centerIn: parent
                            spacing: Theme.paddingLarge
                            visible: dlg.dynAvailable

                            Image {
                                width: Theme.iconSizeLauncher
                                height: width
                                fillMode: Image.PreserveAspectFit
                                sourceSize.width: width
                                sourceSize.height: height
                                cache: false
                                source: (dlg.clockSelected && dlg.hasActiveDynClock)
                                        ? ("image://muoto-launcher/dyn-clock/" + dlg.activePack
                                           + "?t=" + dlg.previewTick)
                                        : (dlg.stockDyn
                                           ? "image://theme/icon-launcher-clock"
                                           : ("image://muoto-launcher/icon-pack/" + dlg.activePack
                                              + "/icon-launcher-clock"))
                            }

                            Image {
                                width: Theme.iconSizeLauncher
                                height: width
                                fillMode: Image.PreserveAspectFit
                                sourceSize.width: width
                                sourceSize.height: height
                                cache: false
                                source: (dlg.calendarSelected && dlg.hasActiveDynCalendar)
                                        ? ("image://muoto-launcher/dyn-calendar/" + dlg.activePack
                                           + "?t=" + dlg.previewTick)
                                        : (dlg.stockDyn
                                           ? "image://theme/icon-launcher-calendar"
                                           : ("image://muoto-launcher/icon-pack/" + dlg.activePack
                                              + "/icon-launcher-calendar"))
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            width: parent.width - Theme.paddingLarge * 2
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            color: Theme.secondaryColor
                            visible: !dlg.dynAvailable
                            text: qsTr("This theme doesn't include live Clock or Calendar icons. Try another theme, or restore the default look.")
                        }
                    }
                }

                Column {
                    width: isLandscape ? parent.width / 2 : parent.width

                    MuotoTextLabel {
                        visible: dlg.dynAvailable
                        text: dlg.stockDyn
                              ? qsTr("Show the current time and date on your Clock and Calendar icons.")
                              : qsTr("Show the current time and date on your Clock and Calendar icons, using this theme's style.")
                    }

                    IconTextSwitch {
                        automaticCheck: false
                        text: qsTr("Dynamic clock icon")
                        description: qsTr("Shows the current time on the Clock launcher icon.")
                        visible: dlg.hasActiveDynClock
                        checked: dlg.clockSelected
                        enabled: dlg.hasActiveDynClock && !settings.isRunning
                        onClicked: dlg.clockSelected = !dlg.clockSelected
                    }

                    IconTextSwitch {
                        automaticCheck: false
                        text: qsTr("Dynamic calendar icon")
                        description: qsTr("Shows today's date on the Calendar launcher icon.")
                        visible: dlg.hasActiveDynCalendar
                        checked: dlg.calendarSelected
                        enabled: dlg.hasActiveDynCalendar && !settings.isRunning
                        onClicked: dlg.calendarSelected = !dlg.calendarSelected
                    }
                }
            }

LabelSpacer { }        }
    }
}
