import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.uithemer 1.0
import org.nemomobile.notifications 1.0
import "../components"

Page
{
    id: welcomepage
    focus: true

    property bool vDep: false
    property bool vDon: false
    property bool vIM: themepack.hasImageMagickInstalled()

    ThemePack { id: themepack }
    BusyState { id: busyindicator }
    Notification { id: notification }

    Keys.onPressed: {
        handleKeyPressed(event);
    }

    function handleKeyPressed(event) {

        if (event.key === Qt.Key_Down) {
            flickable.flick(0, - welcomepage.height);
            event.accepted = true;
        }

        if (event.key === Qt.Key_Up) {
            flickable.flick(0, welcomepage.height);
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

        if (event.key === Qt.Key_G) {
            pageStack.push(Qt.resolvedUrl("GuidePage.qml"));
            event.accepted = true;
        }

    }

    Connections
    {
        function notify() {
            settings.isRunning = false;
            notification.publish();
        }

        target: themepack
        onDependenciesInstalled: {
            vDep = true
            installdep.enabled = false
            itsdep.enabled = false
            notify()
        }

        onImageMagickInstalled: {
            vIM = true
            imagemagick.enabled = false
            notify()
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

            PageHeader { title: qsTr("Welcome to UI Themer") }

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

            LabelText {
                text: qsTr("UI Themer lets you customize icons, fonts and pixel density in Sailfish OS.")
            }

            SectionHeader { text: qsTr("Terms and conditions") }

            LabelText {
                text: qsTr("By using UI Themer, you agree to the <a href='https://www.gnu.org/licenses/gpl-3.0'>GNU GPLv3</a> terms and conditions.")
            }

            LabelText {
                text: qsTr("UI Themer DOES NOT send any data. Some essential info (e.g. the current theme) are collected and stored EXCLUSIVELY locally and used only for the proper functioning of the app (e.g. to display the current theme in the app).")
            }

            SectionHeader { text: qsTr("Usage guide") }

            Button {
                 id: usageguide
                 anchors.horizontalCenter: parent.horizontalCenter
                 text: qsTr("Usage guide")
                 onClicked: pageStack.push(Qt.resolvedUrl("GuidePage.qml"))
             }

            }

            Column
            {
                width: isLandscape ? parent.width/2 : parent.width

            SectionHeader { text: qsTr("Dependencies") }

            LabelText {
                text: qsTr("UI Themer needs some additional dependencies in order to function properly. Install them now if you haven't already.")
            }

            LabelText {
                text: qsTr("It may take a while, do not quit.")
            }

            LabelSpacer { }

             Button {
                  id: installdep
                  anchors.horizontalCenter: parent.horizontalCenter
                  enabled: true
                  text: qsTr("Install dependencies")
                  onClicked: {
                      settings.isRunning = true;
                      themepack.installDependencies();
                  }
              }

            IconTextSwitch {
                id: itsdep
                enabled: true
                automaticCheck: true
                text: qsTr("I have already installed the dependencies")
                checked: false

                onClicked: {
                    if (itsdep.checked) {
                        installdep.enabled = false;
                        vDep = true;
                    } else {
                        installdep.enabled = true;
                        vDep = false;
                    }
                }
            }

            SectionHeader { text: qsTr("ImageMagick") }

            LabelText {
                text: qsTr("ImageMagick is required for UI Themer overlays to work. Overlays need to be supported by the theme.")
            }

            LabelSpacer { }

            Button {
                id: imagemagick
                anchors.horizontalCenter: parent.horizontalCenter
                enabled: vIM ? false : true
                text: vIM ? qsTr("ImageMagick installed") : qsTr("Install ImageMagick")
                onClicked: {
                    settings.isRunning = true;
                    themepack.installImageMagick();
                }
            }

            SectionHeader { text: qsTr("Support") }

            LabelText {
                text: qsTr("If you like my work and want to buy me a beer, feel free to do it!")
            }

            LabelSpacer { }

            Button {
                id: donate
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Donate")
                onClicked: {
                    Qt.openUrlExternally("https://www.paypal.me/fravaccaro");
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

             Button {
                  id: startuit
                  anchors.horizontalCenter: parent.horizontalCenter
                  // enabled: vDep && vDon && vIM
                  text: qsTr("Start UI Themer")
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
