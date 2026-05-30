import Opal.About 1.0 as A
import QtQuick 2.0
import Sailfish.Silica 1.0 as S

A.AboutPageBase {
    id: root

    allowedOrientations: S.Orientation.All
    backNavigation: !settings.isRunning
    showNavigationIndicator: !settings.isRunning
    opacity: settings.isRunning ? 0.2 : 1
    appName: "Muoto"
    appIcon: Qt.resolvedUrl("../../images/appinfo.png")
    appVersion: "3.0.0"
    appRelease: "beta 6"
    description: qsTr("Muoto lets you customize icons, fonts and pixel density in Sailfish OS.")
    authors: ["fravaccaro"]
    homepageUrl: "https://uithemer.github.io/sailfishos-uithemer/"
    sourcesUrl: "https://github.com/uithemer/sailfishos-uithemer"
    donations.text: donations.defaultTextCoffee
    // changelogItems: []
    contributionSections: [
        A.ContributionSection {
            title: qsTr("Contributors")
            groups: [
                A.ContributionGroup {
                    title: qsTr("UI Themer C++")
                    entries: ["Dax89"]
                },
                A.ContributionGroup {
                    title: qsTr("UI Themer Services")
                    entries: ["Eugenio_g7", "LQS"]
                },
                A.ContributionGroup {
                    title: qsTr("UI Themer porting for Sailfish OS 5")
                    entries: ["Lobanokivan11"]
                },
                A.ContributionGroup {
                    title: qsTr("Testing")
                    entries: [qsTr("Community testers")]
                }
            ]
        }
    ]
    donations.services: [
        A.DonationService {
            name: "Liberapay"
            url: "https://liberapay.com/fravaccaro"
        }
    ]
    extraSections: [
        A.InfoSection {
            title: qsTr("Theme packs")
            text: qsTr("If you want to create a theme compatible with Muoto, please read the documentation.")
            buttons: [
                A.InfoButton {
                    text: qsTr("Documentation")
                    onClicked: Qt.openUrlExternally("https://uithemer.github.io/sailfishos-uithemer/docs/getstarted")
                }
            ]
        },
        A.InfoSection {
            title: qsTr("Feedback")
            text: qsTr("If you want to provide feedback or report an issue, please use GitHub.")
            buttons: [
                A.InfoButton {
                    text: qsTr("Issues")
                    onClicked: Qt.openUrlExternally("https://github.com/uithemer/sailfishos-uithemer/issues")
                }
            ]
        },
        A.InfoSection {
            title: qsTr("Translations")
            text: qsTr("Credits for existing translations and how to contribute.")
            buttons: [
                A.InfoButton {
                    text: qsTr("Translations")
                    onClicked: pageStack.push(Qt.resolvedUrl("TranslatorPage.qml"))
                }
            ]
        }
    ]
    attributions: [
        A.Attribution {
            name: "Original Icon pack support GUI"
            entries: ["RikudouSennin"]
            homepage: "https://github.com/RikudouSage/sailfish-iconpacksupport-gui"

        }
    ]

    licenses: A.License {
        spdxId: "GPL-3.0-or-later"
    }

}
