import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    height: previewlabel.height
    width: parent.width

    property string packName: ""
    property string selectedFont: ""

    function fontWeightFromBasename(name) {
        if (!name || name === "")
            return Font.Normal
        var n = name.toLowerCase()
        if (n.indexOf("hairline") >= 0 || n.indexOf("thin") >= 0)
            return Font.Thin
        if (n.indexOf("extralight") >= 0 || n.indexOf("extra-light") >= 0
                || n.indexOf("ultralight") >= 0 || n.indexOf("ultra light") >= 0
                || (n.indexOf("extra") >= 0 && n.indexOf("light") >= 0 && n.indexOf("bold") < 0))
            return Font.ExtraLight
        if (n.indexOf("light") >= 0)
            return Font.Light
        if (n.indexOf("regular") >= 0 || n.indexOf("normal") >= 0 || n.indexOf("book") >= 0)
            return Font.Normal
        if (n.indexOf("medium") >= 0)
            return Font.Medium
        if (n.indexOf("semibold") >= 0 || n.indexOf("semi bold") >= 0
                || n.indexOf("demibold") >= 0 || n.indexOf("demi bold") >= 0)
            return Font.DemiBold
        if (n.indexOf("extrabold") >= 0 || n.indexOf("extra bold") >= 0
                || n.indexOf("ultrabold") >= 0 || n.indexOf("ultra bold") >= 0)
            return Font.ExtraBold
        if (n.indexOf("bold") >= 0)
            return Font.Bold
        if (n.indexOf("black") >= 0 || n.indexOf("heavy") >= 0)
            return Font.Black
        return Font.Normal
    }

FontLoader {
    id: previewfont
    source: (packName && selectedFont)
            ? ("/usr/share/" + packName + "/font/" + selectedFont + ".ttf")
            : ""
}

Label {
    id: previewlabel
    width: parent.width - (x * 2)
    x: Theme.paddingLarge
    height: 350
    font.family: previewfont.name
    font.weight: fontWeightFromBasename(selectedFont)
    text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Maecenas imperdiet finibus venenatis. Suspendisse mollis urna sed luctus sodales."
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    wrapMode: Text.WordWrap
    truncationMode: TruncationMode.Fade
}

}
