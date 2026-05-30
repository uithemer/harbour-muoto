import Nemo.Notifications 1.0
import QtQuick 2.0

Notification {
    id: notificationComponent

    appName: "Muoto"
    appIcon: "/usr/share/icons/hicolor/86x86/apps/harbour-muoto.png"
    category: "x-nemo.muoto"

    function show(title, message) {
        summary = title
        body = message
        progress = undefined
        isTransient = false
        publish()
    }

    function toast(message) {
        summary = ""
        body = message
        progress = undefined
        isTransient = true
        publish()
    }

    function updateProgress(title, message, progressValue) {
        summary = title
        body = message
        progress = progressValue
        isTransient = false
        publish()
    }
}
