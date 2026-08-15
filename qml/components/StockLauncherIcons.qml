import Qt.labs.folderlistmodel 2.1
import QtQuick 2.0
import Sailfish.Silica 1.0

FolderListModel {
    folder: {
        var pr = Theme.pixelRatio;
        var z = "z1.0";
        if (pr >= 2.5)
            z = "z2.5";
        else if (pr >= 2)
            z = "z2.0";
        else if (pr >= 1.75)
            z = "z1.75";
        else if (pr >= 1.5)
            z = "z1.5";
        else if (pr >= 1.25)
            z = "z1.25";
        return "file:///usr/share/themes/sailfish-default/silica/" + z + "/icons";
    }
    nameFilters: ["icon-launcher-*.png"]
    showDirs: false
    showFiles: true
    showHidden: false
    showOnlyReadable: true
    sortField: FolderListModel.Unsorted
}
