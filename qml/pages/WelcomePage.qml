import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.muoto 1.0
import "../components"

Page
{
    property bool vDon: false

    ThemePack { id: themepack }

    SilicaFlickable
    {
        id: flickable
        anchors.fill: parent
        contentHeight: content.height
        enabled: !settings.isRunning
        opacity: settings.isRunning ? 0.2 : 1.0

        VerticalScrollDecorator { }

        Column
        {
            id: content
            width: parent.width

            PageHeader { }
            
            Grid {
                width: parent.width
                columns: isLandscape ? 2 : 1

            Column
            {
                width: isLandscape ? parent.width/2 : parent.width

                Item {
                    height: appicon.height + Theme.paddingMedium
                    width: parent.width
                    Image { id: appicon; anchors.horizontalCenter: parent.horizontalCenter; source: "../../images/appinfo.png" }
                }

            MuotoHeaderLabel {
                text: qsTr("Welcome to Muoto")
            }

            MuotoTextLabel {
                text: qsTr("Muoto lets you customize icons, fonts and pixel density in Sailfish OS.")
            }

            SectionHeader { text: qsTr("Terms and conditions") }

            MuotoTextLabel {
                text: qsTr("By using Muoto, you agree to the <a href='https://www.gnu.org/licenses/gpl-3.0'>GNU GPLv3</a> terms and conditions.")
            }

            MuotoTextLabel {
                text: qsTr("Muoto DOES NOT send any data. Some essential info (e.g. the current theme) are collected and stored EXCLUSIVELY locally and used only for the proper functioning of the app (e.g. to display the current theme in the app).")
            }

            }

            Column
            {
                width: isLandscape ? parent.width/2 : parent.width

            SectionHeader { text: qsTr("Support") }

            MuotoTextLabel {
                text: qsTr("If you like my work and want to buy me a beer, feel free to do it!")
            }

            LabelSpacer { }

            MuotoButton {
                id: donate
                text: qsTr("Donate")
                onClicked: {
                    Qt.openUrlExternally("https://liberapay.com/fravaccaro");
                    vDon = true
                    itsdon.enabled = false
                }
            }

            IconTextSwitch {
                id: itsdon
                enabled: true
                automaticCheck: true
                text: qsTr("I don't care donating")
                checked: false

                onClicked: {
                    if (itsdon.checked) {
                        donate.enabled = false;
                        vDon = true;
                    } else {
                        donate.enabled = true;
                        vDon = false;
                    }
                }
            }

            }
            } // grid

            LabelSpacer { }

            MuotoTextLabel {
                text: qsTr("This software is provided \"as is\", without warranty of any kind, express or implied. In no event shall the authors or copyright holders be liable for any claim, damages or other liability arising from, out of or in connection with the software. You are solely responsible for the safety of your data.")
            }

            IconTextSwitch {
                id: itsdisclaimer
                automaticCheck: true
                text: qsTr("I understand and accept")
                checked: false
            }

             MuotoButton {
                  id: startuit
                  enabled: vDon && itsdisclaimer.checked
                  text: qsTr("Start Muoto")
                  onClicked: {
                      settings.wizardDone = true;
                      pageStack.replace("MainPage.qml");
                  }
              }

             Item {
                 width: parent.width
                 height: Theme.paddingLarge
             }
        }

    }
}
