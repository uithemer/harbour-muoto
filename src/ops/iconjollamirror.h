#ifndef ICONJOLLAMIRROR_H
#define ICONJOLLAMIRROR_H

class IconJollaMirror
{
public:
    // Copy stock icon-launcher-* (not icon-launcher-folder-*) from themes/silica into hicolor (create-if-missing).
    void mirrorStockLauncherIcons() const;
    // Remove mirrored app icon-launcher-* from all hicolor apps tiers (restore).
    void removeStockLauncherIconsFromHicolor() const;
};

#endif // ICONJOLLAMIRROR_H
