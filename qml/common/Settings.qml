import QtQuick 2.0
import harbour.uithemer 1.0
import org.nemomobile.configuration 1.0

Item
{

    ConfigurationGroup {
        id: conf
        path: "/desktop/lipstick/sailfishos-uithemer"
        property bool showGuimode
        property bool showDensity
        property int guimode
        property bool densityEnabled
        property bool wizardDone
        property string activeIconPack
        property string activeFontPack
        property bool coverActiveTheme
        property int coverAction1
        property int coverAction2
        property int autoUpdate
        property bool servicesu
    }

    property alias showGuimode: conf.showGuimode
    property alias showDensity: conf.showDensity
    property alias guimode: conf.guimode
    property alias densityEnabled: conf.densityEnabled
    property alias wizardDone: conf.wizardDone
    property alias activeIconPack: conf.activeIconPack
    property alias activeFontPack: conf.activeFontPack
    property alias coverActiveTheme: conf.coverActiveTheme
    property alias coverAction1: conf.coverAction1
    property alias coverAction2: conf.coverAction2
    property alias autoUpdate: conf.autoUpdate
    property alias servicesu: conf.servicesu

    property bool homeRefresh: true
    property bool isRunning: false

    function deactivateIcon() { activeIconPack = "default"; }
    function deactivateFont() { activeFontPack = "default"; }

    id: settings

    onGuimodeChanged: conf.sync();
    onDensityEnabledChanged: conf.sync();
    onWizardDoneChanged: conf.sync();
    onActiveIconPackChanged: conf.sync();
    onActiveFontPackChanged: conf.sync();
    onCoverActiveThemeChanged: conf.sync();
    onCoverAction1Changed: conf.sync();
    onCoverAction2Changed: conf.sync();
    onAutoUpdateChanged: conf.sync();
    onServicesuChanged: conf.sync();

    Component.onCompleted: {
        conf.sync();
    }

    property string deviceModel: switch (themepack.readDeviceModel()) {
      case "h3113" || "h3123" || "h3133":
          return "XA2";
      case "h4113" || "h4133":
          return "XA2 Dual";
      case "h3413":
          return "XA2 Plus";
      case "h4413" || "h4493":
          return "XA2 Plus Dual";
      case "h3213" || "h3223":
          return "XA2 Ultra";
      case "h4213" || "h4233":
          return "XA2 Ultra Dual";
      }
    property bool isXA2: (deviceModel === "XA2" || deviceModel === "XA2 Dual" || deviceModel === "XA2 Plus" || deviceModel === "XA2 Plus Dual" || deviceModel === "XA2 Ultra" || deviceModel === "XA2 Ultra Dual") ? true : false
}
