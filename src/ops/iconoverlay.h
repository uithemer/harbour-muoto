#ifndef ICONOVERLAY_H
#define ICONOVERLAY_H

#include <QString>

class IconOverlay
{
public:
    bool apply(const QString& packName, bool runPack) const;
};

#endif // ICONOVERLAY_H
