import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0
import org.nemomobile.configuration 1.0
import harbour.uithemer 1.0
import "../components"
import "../components/themepacklistview"

Page
{
    id: densitypage
    focus: true


    RemorsePopup { id: remorsepopup }
    ThemePack { id: themepack }
    BusyState { id: busyindicator }
    Notification { id: notification }

    // 2.6.0: DensityEnabler is no longer a QML type. The vendor
    // dconf-locks relocation requires root, so it goes through the
    // daemon via Helper.densityEnable(); restoreDensity (the inverse
    // operation) is unprivileged and stays in-process inside
    // ThemePackModel::restoreDpi.
    Component.onCompleted: Helper.densityEnable();

    ThemePackModel {
                function applyDone() {
                    notifyDone();
                    if(settings.homeRefresh === true) {
                        themepack.restartHomescreen();
                        console.log("homescreen restart");
                    } else
                        console.log("no homescreen restart");
                }
                function notifyDone() {
                    settings.isRunning = false;
                    notification.publish();
                }

                id: themepackmodel
                onDpiRestored: {
                    silica.sync();
                    sldpr.value = silica.theme_pixel_ratio;
                    cbiz.value = silica.icon_size_launcher;
                    applyDone()
                }
            }

    ConfigurationGroup {
        id: silica
        path: "/desktop/sailfish/silica"
        property real theme_pixel_ratio
        property real icon_size_launcher
    }

    function _restartHomescreenWithRemorse() {
        remorseAction(qsTr("Restarting homescreen"), function() {
            themepack.restartHomescreen();
        });
    }

    Keys.onPressed: {
        handleKeyPressed(event);
    }

    function handleKeyPressed(event) {

        if (event.key === Qt.Key_Down) {
            densityView.flick(0, - densitypage.height);
                    event.accepted = true;
        }

        if (event.key === Qt.Key_Up) {
            densityView.flick(0, densitypage.height);
                    event.accepted = true;
        }

        if (event.key === Qt.Key_PageDown) {
            densityView.scrollToBottom();
                    event.accepted = true;
        }

        if (event.key === Qt.Key_PageUp) {
            densityView.scrollToTop();
                    event.accepted = true;
        }

        if (event.key === Qt.Key_Return) {
            if (remorsepopup.active)
            remorsepopup.trigger();
            event.accepted = true;
        }

        if (event.key === Qt.Key_B) {
            pageStack.navigateBack();
            event.accepted = true;
        }

        if (event.key === Qt.Key_C) {
            remorsepopup.cancel();
            event.accepted = true;
        }

        if (event.key === Qt.Key_H) {
            pageStack.replaceAbove(null, Qt.resolvedUrl("MainPage.qml"));
            event.accepted = true;
        }

        if (event.key === Qt.Key_G) {
            pageStack.push(Qt.resolvedUrl("GuidePage.qml"));
            event.accepted = true;
        }

        if (event.key === Qt.Key_W) {
            settings.wizardDone = false
            pageStack.replaceAbove(null, Qt.resolvedUrl("WelcomePage.qml"));
            event.accepted = true;
        }

        if (event.key === Qt.Key_A) {
            pageStack.replace(Qt.resolvedUrl("AboutPage.qml"));
            event.accepted = true;
        }

        if (event.key === Qt.Key_R) {
            _restartHomescreenWithRemorse();
            event.accepted = true;
        }
    }


    SilicaFlickable {
        id: densityView
        anchors.fill: parent
        contentHeight: densityContent.height
        enabled: !settings.isRunning
        opacity: settings.isRunning ? 0.2 : 1.0

        PullDownMenu {
            MenuItem {
                text: qsTr("Usage guide")
                onClicked: pageStack.push(Qt.resolvedUrl("GuidePage.qml"))
            }
            MenuItem {
                text: qsTr("Restore display density")
                onClicked: {
                    var dlgrestore = pageStack.push("RestoreDDPage.qml", { "settings": settings });

                    dlgrestore.accepted.connect(function() {
                        settings.isRunning = true;
                        themepackmodel.restoreDpi(dlgrestore.restoreDPR, dlgrestore.restoreIconSize);
                    });
                }
            }
        }

        Column
        {
            id: densityContent
            width: parent.width

            PageHeader { title: qsTr("Display density") }

            Grid {
                width: parent.width
                columns: isLandscape ? 2 : 1

            Column
            {
                width: isLandscape ? parent.width/2 : parent.width

            Column
            {
                id: coldpr
                width: parent.width

                SectionHeader { text: qsTr("Device pixel ratio") }

                Slider {
                    id: sldpr
                    width: parent.width
                    label: qsTr("Device pixel ratio")
                    maximumValue: 2.3
                    minimumValue: 0.7
                    stepSize: 0.05
                    value: silica.theme_pixel_ratio
                    valueText: value
                    onPressAndHold: cancel()

                    onReleased: {
                        silica.theme_pixel_ratio = value;
                    }
                }

                LabelText {
                    text: qsTr("Change the display pixel ratio. To a smaller value corresponds an higher density.")
                }
            }

            }

            Column
            {
                width: isLandscape ? parent.width/2 : parent.width

            SectionHeader { text: qsTr("Icon size") }
            Column
            {
                id: coliz
                width: parent.width

                ComboBox {
                    id: cbiz
                    width: parent.width
                    label: qsTr("Icon size")
                    description: qsTr("Change the size of UI icons. To a greater value corresponds an huger size.")
                    value: silica.icon_size_launcher

                    menu: ContextMenu {
                        MenuItem { text: "86"; onClicked: silica.icon_size_launcher = 86 }
                        MenuItem { text: "108"; onClicked: silica.icon_size_launcher = 108 }
                        MenuItem { text: "129"; onClicked: silica.icon_size_launcher = 129 }
                        MenuItem { text: "151"; onClicked: silica.icon_size_launcher = 151 }
                        MenuItem { text: "172"; onClicked: silica.icon_size_launcher = 172 }
                    }
                }
            }

                LabelText {
                    text: "<br>" + qsTr("Remember to restart the homescreen (from the <i>Options</i> page) right after you have changed the settings in this page.")
                }

            }
            } // grid

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }

            VerticalScrollDecorator { }
        }

    }

}
