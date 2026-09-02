import Opal.SupportMe 1.0
import QtQuick 2.0
import Sailfish.Silica 1.0
import "common"
import "components"
import harbour.muoto 1.0
import "pages"

ApplicationWindow {
    id: app

    property bool isLightTheme: (Theme.colorScheme === Theme.LightOnDark) ? false : true

    function showProgressNotification(summary, body, progressValue) {
        globalNotification.updateProgress(summary, body, progressValue);
    }

    function showToast(body) {
        globalNotification.toast(body);
    }

    function formatHelperError(message, emptyFallback) {
        switch (message) {
        case "busy":
            return qsTr("Busy...");
        case "timed out waiting for icon operation":
            return qsTr("Busy — try again in a moment");
        case "shutting down":
            return qsTr("Cannot apply changes while shutting down");
        case "launcher daemon not running":
            return qsTr("Try again in a moment");
        case "upgrade in progress":
            return qsTr("Wait for the system update to finish");
        case "D-Bus interface unavailable":
            return qsTr("Try again in a moment");
        case "failed to relocate one or more vendor locks":
            return qsTr("Could not unlock display density settings");
        case "invalid pack":
            return qsTr("This theme pack cannot be used");
        case "no icon operation":
            return qsTr("No icons to apply");
        case "pack not found":
            return qsTr("Theme pack is not installed");
        case "overlay not applied":
            return qsTr("Could not style missing app icons");
        case "pack run produced no copies":
            return qsTr("No icons could be installed from this theme");
        case "no icons could be written":
            return qsTr("Icons could not be changed — try restarting the device");
        case "some icons could not be updated":
            return qsTr("Some icons could not be updated");
        case "inplace restore failed":
            return qsTr("Some icons could not be restored");
        case "cannot determine the package to remove":
            return qsTr("Could not remove this theme — try again in a moment");
        default:
            return message.length ? qsTr("Something went wrong") : (emptyFallback || qsTr("Operation failed"));
        }
    }

    function showHelperError(message, emptyFallback) {
        showToast(formatHelperError(message, emptyFallback));
    }

    initialPage: settings.wizardDone ? mainpage : welcomepage
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations
    _defaultPageOrientations: defaultAllowedOrientations

    MuotoNotification {
        id: globalNotification
    }

    Component {
        id: mainpage

        MainPage {
        }

    }

    Component {
        id: welcomepage

        WelcomePage {
        }

    }

    IconApplier {
        // Preview only (IconPackPreview buildPreview). Icon apply/restore go
        // through Helper -> session launcher D-Bus.

        id: iconapplier
    }

    Settings {
        id: settings
    }

    AskForSupport {
        id: askForSupport

        contents: Component {
            MuotoSupportDialog {
            }

        }

    }

}
