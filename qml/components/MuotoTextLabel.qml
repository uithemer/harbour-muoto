import QtQuick 2.0
import Sailfish.Silica 1.0

Label {
    topPadding: Theme.paddingMedium
    bottomPadding: Theme.paddingMedium
    x: Theme.horizontalPageMargin
    width: parent.width - 2 * x
    color: palette.highlightColor
    font.pixelSize: Theme.fontSizeMedium
    wrapMode: Text.Wrap
}
