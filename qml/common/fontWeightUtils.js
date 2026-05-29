.import QtQuick 2.0 as QtQuick
.pragma library

function fontBasenameFromFilename(filename) {
    if (!filename || filename === "")
        return ""
    return filename.replace(/\.(ttf|ttc)$/i, "")
}

function fontTtfPath(packName, basename) {
    if (!packName || !basename)
        return ""
    return "/usr/share/" + packName + "/font/" + basename + ".ttf"
}

function fontWeightFromBasename(name) {
    if (!name || name === "")
        return QtQuick.Font.Normal
    var n = name.toLowerCase()
    if (n.indexOf("hairline") >= 0 || n.indexOf("thin") >= 0)
        return QtQuick.Font.Thin
    if (n.indexOf("extralight") >= 0 || n.indexOf("extra-light") >= 0
            || n.indexOf("ultralight") >= 0 || n.indexOf("ultra light") >= 0
            || (n.indexOf("extra") >= 0 && n.indexOf("light") >= 0 && n.indexOf("bold") < 0))
        return QtQuick.Font.ExtraLight
    if (n.indexOf("light") >= 0)
        return QtQuick.Font.Light
    if (n.indexOf("regular") >= 0 || n.indexOf("normal") >= 0 || n.indexOf("book") >= 0)
        return QtQuick.Font.Normal
    if (n.indexOf("medium") >= 0)
        return QtQuick.Font.Medium
    if (n.indexOf("semibold") >= 0 || n.indexOf("semi bold") >= 0
            || n.indexOf("demibold") >= 0 || n.indexOf("demi bold") >= 0)
        return QtQuick.Font.DemiBold
    if (n.indexOf("extrabold") >= 0 || n.indexOf("extra bold") >= 0
            || n.indexOf("ultrabold") >= 0 || n.indexOf("ultra bold") >= 0)
        return QtQuick.Font.ExtraBold
    if (n.indexOf("bold") >= 0)
        return QtQuick.Font.Bold
    if (n.indexOf("black") >= 0 || n.indexOf("heavy") >= 0)
        return QtQuick.Font.Black
    return QtQuick.Font.Normal
}
