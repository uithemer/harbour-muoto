#ifndef ICONJOLLAMIRROR_H
#define ICONJOLLAMIRROR_H

class IconJollaMirror
{
public:
    // Copy stock icon-launcher-* from themes/silica into hicolor (create-if-missing).
    void mirrorStockLauncherIcons() const;
    // Remove icon-launcher-* from all hicolor apps tiers (restore).
    void removeStockLauncherIconsFromHicolor() const;
};

#endif // ICONJOLLAMIRROR_H
