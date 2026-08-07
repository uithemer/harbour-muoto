#ifndef ICONPACKFACTORY_H
#define ICONPACKFACTORY_H

#include <QList>

class IconPack;

class IconPackFactory
{
public:
    static QList<IconPack*> loadIconPacks();
};

#endif // ICONPACKFACTORY_H
