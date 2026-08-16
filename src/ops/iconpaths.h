#ifndef ICONPATHS_H
#define ICONPATHS_H

#include <QString>
#include <QStringList>

namespace IconPaths
{
    QString muotoShare();
    QString backupIconsRoot();
    QString packDir(const QString& packName);
    QString resolvePackCapabilityDir(const QString& packRoot, const QString& capability);
    bool packCapabilityUsable(const QString& packRoot, const QString& capability);

    const QStringList& jollaSizes();

    QString liveJollaIconsDir(const QString& zSize);
}

#endif // ICONPATHS_H
