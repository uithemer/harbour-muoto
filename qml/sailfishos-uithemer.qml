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
        // /home/defaultuser/.local/share/applications, so turning it
        // on is unprivileged. When the debounced rescan fires it
        // emits watcherFired() (see Connections below); the actual
        // rescan -- drift reassert + uninstall cleanup + new-theming
        // -- runs in the daemon, which is the only side with the
        // privilege to rewrite /usr/share/applications/*.desktop and
        // the system manifest. On startup we also fire one catch-up
        // call so apps installed while the GUI was closed get themed.
        Component.onCompleted: {
            enableAutoTheming(true);
            Helper.themeNewDesktops(settings.iconOverlay);
        }
    }
    Connections {
        target: iconapplier
        // Watcher fired => ask the daemon to rescan, passing the
        // user's last-known overlay preference (settings.iconOverlay
        // is set on ApplyIcons and cleared on RestoreIcons/uninstall).
        // The daemon's busy gate self-drops the call if a heavy op is
        // already in flight, so we don't worry about racing with it.
        onWatcherFired: Helper.themeNewDesktops(settings.iconOverlay)
    }
    // Debounce a burst of watcher fires (one APK update can trigger
    // several directoryChanged events on apkd-bridge's behalf, and
    // the systemd path unit may pile on its own call) so we only
    // restart lipstick once per quiet window.
    Timer {
        id: rescanRestart
        interval: 2000
        repeat: false
        onTriggered: themepack.restartHomescreen()
    }
    Connections {
        target: Helper
        // Daemon emits count = themed + reasserted (entries whose
        // Icon= changed). settings.isRunning guard: an explicit apply
        // calls themepack.restartHomescreen() from MainPage._finalise()
        // already; we skip here to avoid a double-restart.
        onNewDesktopsThemed: {
            if(count > 0 && settings.homeRefresh && !settings.isRunning)
                rescanRestart.restart();
        }
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
