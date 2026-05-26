#ifndef ICONOVERLAY_H
#define ICONOVERLAY_H

#include <QString>

class IconOverlay
{
public:
    bool applySfos(const QString& packName) const;
    bool applyApk(const QString& packName, bool* apkIconsTouched = nullptr) const;
};

#endif // ICONOVERLAY_H
