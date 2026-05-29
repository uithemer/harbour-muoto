import QtQuick 2.0
import Sailfish.Silica 1.0

Label {
    topPadding: Theme.paddingMedium
    x: Theme.horizontalPageMargin
    width: parent.width - 2 * x
    horizontalAlignment: Text.AlignHCenter
    font.pixelSize: Theme.fontSizeHuge
    font.family: Theme.fontFamilyHeading
    wrapMode: Text.Wrap
    color: palette.highlightColor
}
