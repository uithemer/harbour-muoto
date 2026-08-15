import QtQuick 2.0
import Sailfish.Silica 1.0

BusyIndicator {
    running: settings.isRunning
    size: BusyIndicatorSize.Large
    anchors.centerIn: parent
}
