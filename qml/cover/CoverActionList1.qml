import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.uithemer 1.0

CoverActionList {
    iconBackground: true
    enabled: ((settings.activeIconPack === 'default') || (settings.isRunning) || (settings.coverAction1 === 3)) ? false : true
    CoverAction {
        iconSource: switch (settings.coverAction1) {
                    case 0:
                        return "image://theme/icon-cover-sync";
                    case 1:
                        return "image://theme/icon-cover-refresh";
                    case 2:
                        return "image://theme/icon-cover-cancel";
                    }
        onTriggered: {
            settings.isRunning = true
            switch (settings.coverAction1) {
            case 0:
                return Helper.reassertIcons();
            case 1:
                return themepack.restartHomescreen();
            case 2:
                return themepackmodel.ocr();
            }
        }
    }
}
