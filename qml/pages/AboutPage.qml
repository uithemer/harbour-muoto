import Opal.About 1.0 as A
import QtQuick 2.0
import Sailfish.Silica 1.0 as S

A.AboutPageBase {
    id: root

    allowedOrientations: S.Orientation.All
    backNavigation: !settings.isRunning
    showNavigationIndicator: !settings.isRunning
    opacity: settings.isRunning ? 0.2 : 1
    appName: "UI Themer"
    appIcon: Qt.resolvedUrl("../../images/appinfo.png")
    appVersion: "3.0.0"
    appRelease: "beta 2"
    description: qsTr("UI Themer lets you customize icons, fonts and pixel density in Sailfish OS.") + "<br><br>" + qsTr("Released under the <a href='https://www.gnu.org/licenses/gpl-3.0'>GNU GPLv3</a> license.")
    authors: ["Francesco Vaccaro"]
    homepageUrl: "https://uithemer.github.io/sailfishos-uithemer/"
    sourcesUrl: "https://github.com/uithemer/sailfishos-uithemer"
    translationsUrl: "https://www.transifex.com/fravaccaro/ui-themer"
    donations.text: donations.defaultTextCoffee
    changelogItems: [
        A.ChangelogItem {
            version: "3.0.0beta2"
            date: new Date(2026, 4, 26)
            paragraphs: qsTr("Opal.About page and tabbed main UI (Themes + Display density). " + "Icon paths: pack jolla/ to silica z/icons/, APK to launcherIcon/. " + "Keyboard shortcuts removed.")
        },
        A.ChangelogItem {
            version: "3.0.0beta2"
            date: new Date(2026, 4, 23)
            paragraphs: qsTr("Icon pipeline: stock mirror skips icon-launcher-folder-*; " + "homescreen restart uses remorse when enabled; live APK theming path.")
        },
        A.ChangelogItem {
            version: "2.7.1"
            date: new Date(2026, 4, 23)
            paragraphs: qsTr("Settings moved to /apps/sailfishos-uithemer dconf namespace; " + "user dconf via defaultuser from helperd/GUI.")
        }
    ]
    contributionSections: [
        A.ContributionSection {
            title: qsTr("Contributors")
            groups: [
                A.ContributionGroup {
                    title: qsTr("Code")
                    entries: ["Dax89"]
                },
                A.ContributionGroup {
                    title: qsTr("Services")
                    entries: ["Eugenio_g7", "LQS"]
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
            name: "PayPal"
            url: "https://www.paypal.me/fravaccaro"
        }
    ]
    extraSections: [
        A.InfoSection {
            title: qsTr("Theme packs")
            text: qsTr("If you want to create a theme compatible with UI Themer, please read the documentation.")
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
                    text: qsTr("Translator credits")
                    onClicked: pageStack.push(Qt.resolvedUrl("TranslatorPage.qml"))
                }
            ]
        }
    ]
    attributions: [
        A.Attribution {
            name: "Icon pack support GUI"
            entries: ["RikudouSennin"]
            homepage: "https://github.com/RikudouSage/sailfish-iconpacksupport-gui"

            licenses: A.License {
                spdxId: "GPL-3.0-or-later"
            }

        },
        A.Attribution {
            name: "App icon"
            entries: ["Free Vectors"]
            homepage: "http://www.freevectors.com/blue-painting-roller/"
        },
        A.Attribution {
            name: "Iconography"
            entries: ["Retinaicons"]
            homepage: "https://www.flaticon.com/authors/retinaicons"
        }
    ]

    licenses: A.License {
        spdxId: "GPL-3.0-or-later"
    }

}
