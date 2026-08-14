.import QtQuick 2.0 as QtQuick
.pragma library

function fontBasenameFromFilename(filename) {
    if (!filename || filename === "")
        return ""
    return filename.replace(/\.(ttf|ttc)$/i, "")
}

function fontTtfPath(packName, basename) {
    if (packName === "default")
        return "file:///usr/share/fonts/sail-sans-pro/SailSansPro-Light.ttf"
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

// Pick a readable sample weight from a FontWeightModel (Regular/Light/Thin…).
function pickPreferredBasename(model) {
    if (!model || model.rowCount() === 0)
        return ""
    var prefs = ["regular", "light", "thin", "book", "normal", "extralight", "medium"]
    for (var p = 0; p < prefs.length; ++p) {
        for (var r = 0; r < model.rowCount(); ++r) {
            var idx = model.index(r, 0)
            if (!idx || !idx.valid)
                continue
            var w = model.data(idx, 257)
            if (w && String(w).toLowerCase().indexOf(prefs[p]) >= 0)
                return w
        }
    }
    return fontBasenameFromFilename(model.firstWeight)
}

function basenameExistsInModel(model, basename) {
    if (!basename || basename === "" || !model || model.rowCount() === 0)
        return false
    for (var r = 0; r < model.rowCount(); ++r) {
        var idx = model.index(r, 0)
        if (!idx || !idx.valid)
            continue
        if (model.data(idx, 257) === basename)
            return true
    }
    return false
}

// Read weight basename from live fontconfig (legacy installs before dconf key).
function activeWeightFromMuotoConf() {
    var xhr = new XMLHttpRequest()
    xhr.open("GET", "file:///home/defaultuser/.config/fontconfig/conf.d/99-muoto.conf", false)
    try {
        xhr.send()
        if (xhr.responseText && xhr.responseText !== "") {
            var m = xhr.responseText.match(/weight '([^']+)'/)
            if (m && m.length > 1)
                return m[1]
        }
    } catch (e) {
    }
    return ""
}
