import Nemo.Notifications 1.0
import QtQuick 2.0

Notification {
    id: notificationComponent

    function toast(message) {
        summary = "";
        body = message;
        progress = undefined;
        isTransient = true;
        publish();
    }

    function updateProgress(title, message, progressValue) {
        summary = title;
        body = message;
        progress = progressValue;
        isTransient = false;
        publish();
    }

    appName: "Muoto"
    appIcon: "/usr/share/icons/hicolor/86x86/apps/harbour-muoto.png"
    category: "x-nemo.muoto"
}
