import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.uithemer 1.0
import "pages"
import "common"

ApplicationWindow
{
    id: app
    property string coverMode
    property string coverIconPreviewPack: ""
    property bool coverIconPreviewOk: false
    property int coverIconPreviewSeq: 0

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
        // Preview only (ConfirmPage buildPreview). Icon apply/restore and
        // cover refresh go through Helper -> helperd.
    }

    Connections {
        target: iconapplier
        onPreviewReady: {
            coverIconPreviewPack = packName
            coverIconPreviewOk = ok
            coverIconPreviewSeq++
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
