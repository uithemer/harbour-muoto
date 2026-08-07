import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Lipstick 1.0
import harbour.muoto 1.0
import Nemo.Configuration 1.0
import "../common"

Page {
    id: page

    property Settings settings

    function completeBaseName(filePath) {
        var fileName = filePath.split('/').pop()
        var to = fileName.lastIndexOf(".")
        return to === -1 ? fileName : fileName.substr(0, to)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTr("Customize launcher icons")
            }

            Item {
                width: parent.width
                height: gridView.height

                ApplicationsGridView {
                    id: gridView
                    height: cellHeight * Math.ceil(count / columns)
                    interactive: false

                    delegate: LauncherGridItem {
                        id: appItem

                        readonly property string packageName: completeBaseName(model.filePath)
                        readonly property bool customized: provider.value !== ""

                        width: gridView.cellWidth
                        height: gridView.cellHeight
                        icon: model.iconId
                        text: model.name

                        onClicked: {
                            var dialog = pageStack.push(Qt.resolvedUrl("ChooseLauncherIconDialog.qml"), {
                                "packName": settings.activeIconPack,
                                "desktopBaseName": packageName
                            })
                            dialog.accepted.connect(function() {
                                provider.value = dialog.providerUri
                            })
                        }

                        Image {
                            anchors {
                                top: parent.top
                                right: parent.right
                            }
                            source: "image://theme/icon-s-asterisk"
                            visible: customized
                        }

                        ConfigurationValue {
                            id: provider
                            key: "/apps/harbour-muoto/launcher/applications/" + packageName + "/provider"
                            defaultValue: ""
                        }
                    }
                }
            }
        }
    }

    LauncherIconHelper {
        id: iconHelper
        iconPackName: settings.activeIconPack
    }
}
