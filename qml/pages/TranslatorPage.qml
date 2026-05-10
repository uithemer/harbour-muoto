import QtQuick 2.0
import Sailfish.Silica 1.0
import "../components"

Page
{
    id: translatorpage
    focus: true
    backNavigation: !settings.isRunning
    showNavigationIndicator: !settings.isRunning
    BusyState { id: busyindicator }

    Keys.onPressed: {
        handleKeyPressed(event);
    }

    function handleKeyPressed(event) {

        if (event.key === Qt.Key_Down) {
            flickable.flick(0, - translatorpage.height);
            event.accepted = true;
        }

        if (event.key === Qt.Key_Up) {
            flickable.flick(0, translatorpage.height);
            event.accepted = true;
        }

        if (event.key === Qt.Key_PageDown) {
            flickable.scrollToBottom();
            event.accepted = true;
        }

        if (event.key === Qt.Key_PageUp) {
            flickable.scrollToTop();
            event.accepted = true;
        }

        if (event.key === Qt.Key_B) {
            pageStack.navigateBack();
            event.accepted = true;
        }

        if (event.key === Qt.Key_H) {
            pageStack.replaceAbove(null, Qt.resolvedUrl("MainPage.qml"));
            event.accepted = true;
        }

        if (event.key === Qt.Key_D) {
            pageStack.replace(Qt.resolvedUrl("DensityPage.qml"));
            event.accepted = true;
        }

        if (event.key === Qt.Key_O) {
            pageStack.replace(Qt.resolvedUrl("OptionsPage.qml"));
            event.accepted = true;
        }

        if (event.key === Qt.Key_G) {
            pageStack.push(Qt.resolvedUrl("GuidePage.qml"));
            event.accepted = true;
        }

        if (event.key === Qt.Key_A) {
            pageStack.navigateBack();
            event.accepted = true;
        }

        if (event.key === Qt.Key_W) {
            settings.wizardDone = false
            pageStack.replaceAbove(null, Qt.resolvedUrl("WelcomePage.qml"));
            event.accepted = true;
        }
    }

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

            PageHeader { title: qsTr("Translations") }

            Grid {
                width: parent.width
                columns: isLandscape ? 2 : 1

            Column
            {
                width: isLandscape ? parent.width/2 : parent.width

                DetailItem {
                    label: "Deutsch"
                    value: "Sailfishman" + "\n" + "mosen" + "\n" + "Jan Heinrich"
                }

                DetailItem {
                    label: "ελληνικά (Greek)"
                    value: "memphisx"
                }

                DetailItem {
                    label: "Español"
                    value: "mad_soft"
                }

                DetailItem {
                    label: "Español (España)"
                    value: "mad_soft"
                }

                DetailItem {
                    label: "Français"
                    value: "Ohaneje Emeka" + "\n" + "Cédric Heintz"
                }

                DetailItem {
                    label: "Italiano"
                    value: "Francesco Vaccaro"
                }

                DetailItem {
                    label: "Magyar"
                    value: "Szabó G."
                }

                DetailItem {
                    label: "Nederlands"
                    value: "Nathan Follens"
                }
        }

        Column
        {
            width: isLandscape ? parent.width/2 : parent.width

            DetailItem {
                label: "Neerlandais (Belgique)"
                value: "Nathan Follens"
            }

              DetailItem {
                  label: "Polski"
                  value: "Tomasz Amborski"
              }

              DetailItem {
                  label: "русский (Russian)"
                  value: "Oleh Ampilohov"
              }

              DetailItem {
                  label: "Svenska"
                  value: "Åke Engelbrektson"
              }

              DetailItem {
                  label: "Zhōngwén (Chinese)"
                  value: "rui kon"
              }

              LabelText {
                  text: qsTr("Request a new language or contribute to existing languages on the Transifex project page.")
              }

            LabelSpacer { }

              Button {
                  anchors.horizontalCenter: parent.horizontalCenter
                  text: qsTr("Transifex")
                  onClicked: Qt.openUrlExternally("https://www.transifex.com/fravaccaro/ui-themer")
              }

        }
    } // grid

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }
        }

    }
}
