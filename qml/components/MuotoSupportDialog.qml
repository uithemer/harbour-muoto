import QtQuick 2.0
import Opal.SupportMe 1.0

SupportDialog {
    greeting: qsTr("Hi there!")
    hook: qsTr("Thank you for using Muoto! If you find it useful, "
               + "consider supporting its development.")
    goodbye: qsTr("Thank you for your support!")

    SupportAction {
        icon: Qt.resolvedUrl("../../images/support-liberapay.png")
        title: qsTr("Donate via Liberapay")
        description: qsTr("Send a tip or recurring support on Liberapay.")
        link: "https://liberapay.com/fravaccaro"
    }

    SupportAction {
        icon: Qt.resolvedUrl("../../images/support-translate.png")
        title: qsTr("Help with translations")
        description: qsTr("Improve Muoto in your language on Transifex.")
        link: "https://explore.transifex.com/fravaccaro/ui-themer"
    }

    SupportAction {
        icon: Qt.resolvedUrl("../../images/support-git.png")
        title: qsTr("Report issues on GitHub")
        description: qsTr("File bugs, suggest features, or follow development.")
        link: "https://github.com/uithemer/harbour-muoto/issues"
    }

    DetailsDrawer {
        title: qsTr("Why support this app?")

        DetailsParagraph {
            text: qsTr("Muoto is free software (GPLv3) for customizing Sailfish "
                       + "icons, fonts, and display density. It is maintained in "
                       + "spare time alongside other projects.")
        }

        DetailsParagraph {
            text: qsTr("Donations and contributions help keep the app compatible "
                       + "with new Sailfish releases and theme packs from the community.")
        }
    }

    DetailsDrawer {
        title: qsTr("Other ways to help")

        DetailsParagraph {
            text: qsTr("Share theme packs, write documentation, test beta builds, "
                       + "or star the project on GitHub — every bit helps.")
        }
    }
}
