import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0

Notification
{
     id: notification
     category: "x-nemo.muoto"
     appName: "Muoto"
     appIcon: "/usr/share/icons/hicolor/86x86/apps/harbour-muoto.png"
     previewSummary: "Muoto"
     previewBody: qsTr("Settings applied.")
     itemCount: 1
     expireTimeout: 5000
     remoteActions: [ {
         "name": "default",
         "service": "org.nemomobile.muoto",
         "path": "/done",
         "iface": "org.nemomobile.muoto",
         "method": "themeApplied"
     } ]
 }
