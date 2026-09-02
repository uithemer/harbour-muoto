#ifndef LAUNCHERMANIFEST_H
#define LAUNCHERMANIFEST_H

#include "muotolauncherglobal.h"
#include <QList>
#include <QString>

struct LauncherManifestEntry
{
    QString desktop;
    QString originalIcon;
    QString themedPath;
    QString mode; // "redirect" or "inplace"
};

class MUOTO_LAUNCHER_EXPORT LauncherManifest
{
public:
    static bool load(QList<LauncherManifestEntry>* out);
    static bool appendEntry(const LauncherManifestEntry& entry);
    /** Swap all of a desktop's entries for a new set, e.g. one per themed
     *  hicolor slot. No-op (and no manifest rewrite) when nothing changed. */
    static bool replaceEntriesForDesktop(const QString& desktopPath,
                                         const QList<LauncherManifestEntry>& newEntries);
    static bool removeEntryForDesktop(const QString& desktopPath);
    static bool restoreAll();
    static void pruneOrphans(const QStringList& existingDesktops);

private:
    static bool save(const QList<LauncherManifestEntry>& entries);
    static void clear();
};

#endif // LAUNCHERMANIFEST_H
