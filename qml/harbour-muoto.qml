import Nemo.Notifications 1.0
import QtQuick 2.0
import Sailfish.Silica 1.0
import Opal.SupportMe 1.0
import harbour.muoto 1.0
import "pages"
import "common"
import "components"

ApplicationWindow
{
    id: app

    MuotoNotification { id: globalNotification }

    function showNotification(summary, body) {
        globalNotification.show(summary, body)
    }

    function showProgressNotification(summary, body, progressValue) {
        globalNotification.updateProgress(summary, body, progressValue)
    }

    function showToast(body) {
        globalNotification.toast(body)
    }

    function formatHelperError(message, emptyFallback) {
        switch (message) {
        case "busy":
            return qsTr("Busy...")
        case "shutting down":
            return qsTr("Cannot apply changes while shutting down")
        case "upgrade in progress":
            return qsTr("Wait for the system update to finish")
        case "D-Bus interface unavailable":
            return qsTr("Try again in a moment")
        case "failed to relocate one or more vendor locks":
            return qsTr("Could not unlock display density settings")
        case "invalid pack":
            return qsTr("This theme pack cannot be used")
        case "no icon operation":
            return qsTr("No icons to apply")
        case "pack not found":
            return qsTr("Theme pack is not installed")
        case "overlay not applied":
            return qsTr("Could not style missing app icons")
        case "pack run produced no copies":
            return qsTr("No icons could be installed from this theme")
        default:
            return message.length ? qsTr("Something went wrong")
                                  : (emptyFallback || qsTr("Operation failed"))
        }
    }

    function showHelperError(message, emptyFallback) {
        showToast(formatHelperError(message, emptyFallback))
    }
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
        // Preview only (IconPackPreview buildPreview). Icon apply/restore go
        // through Helper -> session launcher D-Bus.
    }

    property bool isLightTheme: (Theme.colorScheme === Theme.LightOnDark) ? false : true

    function showSupportDialog() {
        askForSupport.show()
    }

    initialPage: settings.wizardDone ? mainpage : welcomepage
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    allowedOrientations: defaultAllowedOrientations
    _defaultPageOrientations: defaultAllowedOrientations

    Settings { id: settings }

    AskForSupport {
        id: askForSupport
        contents: Component {
            MuotoSupportDialog {}
        }
    }
}
