import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0

Dialog {
    id: dlg

    property string packName
    property string desktopBaseName
    property string providerUri: ""

    LauncherIconHelper {
        id: iconHelper
        iconPackName: packName
    }

    SilicaGridView {
        anchors.fill: parent
        model: iconHelper.packIconIds()
        cellWidth: Theme.iconSizeLarge + Theme.paddingMedium
        cellHeight: cellWidth

        delegate: Image {
            width: Theme.iconSizeLarge
            height: Theme.iconSizeLarge
            source: "image://muoto-launcher/icon-pack/" + packName + "/" + modelData

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    providerUri = "icon-pack://" + packName + "/" + modelData
                    dlg.accept()
                }
            }
        }

        header: Column {
            width: parent.width

            DialogHeader {
                title: qsTr("Choose icon")
            }

            Button {
                text: qsTr("Use default")
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    providerUri = ""
                    dlg.accept()
                }
            }
        }
    }
}
