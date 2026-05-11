import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.uithemer 1.0
import "pages"
import "common"

ApplicationWindow
{
    id: app
    property string coverMode

    Component {
        id: mainpage
        MainPage {}
    }

    Component {
        id: welcomepage
        WelcomePage {}
}

    ThemePack { id: themepack }
    IconApplier {
        id: iconapplier
        // The QFileSystemWatcher itself runs as defaultuser and only
        // observes paths under /usr/share/applications and
        // /home/defaultuser/.local/share/apkd-bridge/launcherIcon, so
        // turning it on is unprivileged. When it fires it calls
        // Helper.themeNewDesktops() (privileged via the daemon).
        Component.onCompleted: enableAutoTheming(true)
        onNewDesktopsThemed: Helper.themeNewDesktops()
    }
    property bool isLightTheme: (Theme.colorScheme === Theme.LightOnDark) ? false : true

    initialPage: settings.wizardDone ? mainpage : welcomepage
    cover: switch (app.coverMode) {
           case "confirmDialog":
               return Qt.resolvedUrl("cover/CoverConfirm.qml");
           default:
               return Qt.resolvedUrl("cover/CoverPage.qml")
           }

    allowedOrientations: defaultAllowedOrientations
    _defaultPageOrientations: defaultAllowedOrientations

    Settings { id: settings }
}
