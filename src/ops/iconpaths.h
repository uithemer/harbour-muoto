#ifndef ICONPATHS_H
#define ICONPATHS_H

#include <QString>
#include <QStringList>

namespace IconPaths
{
    QString muotoShare();
    QString backupIconsRoot();
    QString packDir(const QString& packName);
    // Accepts either the full rpm name or the bare form; returns the bare form,
    // which is what IconPack::name() holds.
    QString packShortName(const QString& packName);
    QString resolvePackCapabilityDir(const QString& packRoot, const QString& capability);
    bool packCapabilityUsable(const QString& packRoot, const QString& capability);

    const QStringList& jollaSizes();

    QString liveJollaIconsDir(const QString& zSize);
}

#endif // ICONPATHS_H
