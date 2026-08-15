import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: root

    property string packName: "default"
    property bool stockLook: true
    property bool hasDynClock: false
    property bool hasDynCalendar: false
    property bool clockLive: false
    property bool calendarLive: false
    property int iconPx: Theme.iconSizeLauncher
    property string previewTick: "0"
    readonly property int shownIconPx: {
        var cap = height;
        if (cap < 8)
            cap = 8;

        var px = iconPx;
        if (px > cap)
            px = cap;

        return px;
    }
    readonly property string clockSource: {
        if (clockLive && hasDynClock)
            return "image://muoto-launcher/dyn-clock/" + packName + "?t=" + previewTick;

        if (stockLook)
            return "image://theme/icon-launcher-clock";

        return "image://muoto-launcher/icon-pack/" + packName + "/icon-launcher-clock";
    }
    readonly property string calendarSource: {
        if (calendarLive && hasDynCalendar)
            return "image://muoto-launcher/dyn-calendar/" + packName + "?t=" + previewTick;

        if (stockLook)
            return "image://theme/icon-launcher-calendar";

        return "image://muoto-launcher/icon-pack/" + packName + "/icon-launcher-calendar";
    }

    width: parent ? parent.width : implicitWidth
    height: parent ? parent.height : implicitHeight

    Row {
        anchors.centerIn: parent
        spacing: Theme.paddingMedium

        Image {
            width: root.shownIconPx
            height: width
            fillMode: Image.PreserveAspectFit
            sourceSize.width: width
            sourceSize.height: height
            cache: false
            source: root.clockSource
        }

        Image {
            width: root.shownIconPx
            height: width
            fillMode: Image.PreserveAspectFit
            sourceSize.width: width
            sourceSize.height: height
            cache: false
            source: root.calendarSource
        }

    }

}
