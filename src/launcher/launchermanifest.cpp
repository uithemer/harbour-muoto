#include "launchermanifest.h"
#include "desktopfile.h"
#include "iconbackup.h"
#include "launcherpaths.h"

#include <MGConfItem>

#include <sys/stat.h>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QStandardPaths>

namespace {

LauncherManifestEntry entryFromJson(const QJsonObject& obj)
{
    LauncherManifestEntry e;
    e.desktop = obj.value(QStringLiteral("desktop")).toString();
    e.originalIcon = obj.value(QStringLiteral("originalIcon")).toString();
    e.themedPath = obj.value(QStringLiteral("themedPath")).toString();
    e.mode = obj.value(QStringLiteral("mode")).toString();
    return e;
}

QJsonObject entryToJson(const LauncherManifestEntry& e)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("desktop"), e.desktop);
    obj.insert(QStringLiteral("originalIcon"), e.originalIcon);
    obj.insert(QStringLiteral("themedPath"), e.themedPath);
    obj.insert(QStringLiteral("mode"), e.mode);
    return obj;
}

QString normalizePath(const QString& path)
{
    return QCryptographicHash::hash(path.toLatin1(), QCryptographicHash::Sha1).toHex();
}

bool restoreInplaceStock(const QString& iconPath)
{
    return IconBackup::restore(iconPath);
}

bool touchPath(const QString& path)
{
    QFile file(path);
    if(!file.exists() || !file.open(QIODevice::Append))
        return false;
    return futimens(file.handle(), nullptr) == 0;
}

} // namespace

bool LauncherManifest::load(QList<LauncherManifestEntry>* out)
{
    if(!out)
        return false;

    out->clear();
    QFile file(LauncherPaths::manifestPath());
    if(!file.exists())
        return true;

    if(!file.open(QIODevice::ReadOnly))
        return false;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if(!doc.isArray())
        return false;

    const QJsonArray arr = doc.array();
    for(const QJsonValue& v : arr)
    {
        if(!v.isObject())
            continue;
        out->append(entryFromJson(v.toObject()));
    }
    return true;
}

bool LauncherManifest::save(const QList<LauncherManifestEntry>& entries)
{
    QJsonArray arr;
    for(const LauncherManifestEntry& e : entries)
        arr.append(entryToJson(e));

    const QJsonDocument doc(arr);
    const QByteArray content = doc.toJson(QJsonDocument::Compact);

    QDir().mkpath(LauncherPaths::muotoShare());
    const QString path = LauncherPaths::manifestPath();

    // Truncate-in-place would leave the manifest empty if the write failed part
    // way, and an empty manifest makes load() fail, which makes restoreAll()
    // restore nothing at all. This is the only record of what to put back, and
    // the dynamic tick rewrites it every minute, so stage and rename instead.
    const QString tmpPath = path + QStringLiteral(".muoto-write");
    QFile::remove(tmpPath);

    {
        QFile tmp(tmpPath);
        if(!tmp.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            qWarning() << "muoto-launcher: could not stage manifest at" << tmpPath;
            return false;
        }
        if(tmp.write(content) != content.size())
        {
            qWarning() << "muoto-launcher: short write staging manifest";
            tmp.close();
            QFile::remove(tmpPath);
            return false;
        }
        tmp.flush();
    }

    QFile::remove(path);
    if(!QFile::rename(tmpPath, path))
    {
        qWarning() << "muoto-launcher: could not place manifest at" << path;
        QFile::remove(tmpPath);
        return false;
    }

    return true;
}

bool LauncherManifest::appendEntry(const LauncherManifestEntry& entry)
{
    QList<LauncherManifestEntry> entries;
    if(!load(&entries))
        return false;

    // The dynamic tick calls this every 60 s with an entry that has not changed.
    // Rewriting the whole manifest 1440 times a day for nothing is both wasted
    // work and 1440 chances to lose it.
    for(const LauncherManifestEntry& existing : entries)
    {
        if(existing.desktop == entry.desktop
           && existing.originalIcon == entry.originalIcon
           && existing.themedPath == entry.themedPath
           && existing.mode == entry.mode)
        {
            return true;
        }
    }

    for(int i = entries.size() - 1; i >= 0; --i)
    {
        if(entries.at(i).desktop == entry.desktop)
            entries.removeAt(i);
    }

    entries.append(entry);
    return save(entries);
}

bool LauncherManifest::replaceEntriesForDesktop(const QString& desktopPath,
                                                const QList<LauncherManifestEntry>& newEntries)
{
    QList<LauncherManifestEntry> entries;
    if(!load(&entries))
        return false;

    QList<LauncherManifestEntry> kept;
    QList<LauncherManifestEntry> existing;
    for(const LauncherManifestEntry& e : entries)
    {
        if(e.desktop == desktopPath)
            existing.append(e);
        else
            kept.append(e);
    }

    // Re-applies rewrite an identical slot set; skip the rewrite then, for the
    // same reason appendEntry short-circuits the dynamic tick.
    if(existing.size() == newEntries.size())
    {
        bool same = true;
        for(int i = 0; i < existing.size() && same; ++i)
        {
            const LauncherManifestEntry& a = existing.at(i);
            const LauncherManifestEntry& b = newEntries.at(i);
            same = a.desktop == b.desktop && a.originalIcon == b.originalIcon
                   && a.themedPath == b.themedPath && a.mode == b.mode;
        }
        if(same)
            return true;
    }

    kept.append(newEntries);
    return save(kept);
}

bool LauncherManifest::removeEntryForDesktop(const QString& desktopPath)
{
    QList<LauncherManifestEntry> entries;
    if(!load(&entries))
        return false;

    bool changed = false;
    for(int i = entries.size() - 1; i >= 0; --i)
    {
        if(entries.at(i).desktop == desktopPath)
        {
            entries.removeAt(i);
            changed = true;
        }
    }

    return changed ? save(entries) : true;
}

bool LauncherManifest::restoreAll()
{
    QList<LauncherManifestEntry> entries;
    if(!load(&entries))
        return false;

    int failures = 0;
    for(const LauncherManifestEntry& e : entries)
    {
        const QFileInfo info(e.desktop);
        MGConfItem saved(LauncherPaths::savedIconKey(info.completeBaseName()));

        // An app uninstalled while themed leaves its entry behind. That is not a
        // restore failure: counting it as one keeps launcher-backup on disk and,
        // via ThemeWork, cancels a pack uninstall.
        if(!QFile::exists(e.desktop))
        {
            qInfo() << "muoto-launcher: manifest restore skipping vanished" << e.desktop;
            if(e.mode == QLatin1String("redirect") && !e.themedPath.isEmpty())
                QFile::remove(e.themedPath);
            saved.set(QString());
            continue;
        }

        if(e.mode == QLatin1String("redirect"))
        {
            DesktopFile desktop(e.desktop);
            desktop.setIcon(e.originalIcon);
            if(!desktop.save())
            {
                qWarning() << "muoto-launcher: manifest restore failed for" << e.desktop;
                ++failures;
            }

            // Do NOT delete e.themedPath here. Lipstick is still painting that
            // PNG and processes the Icon= flip only after its holdback; yanking
            // the file underneath it leaves the tile deaf on stock artwork until
            // a homescreen restart (reproduced on device: restore+apply left
            // Pure Maps and Storeman stock). It becomes unreferenced once the
            // manifest clears, and reconcileGeneratedIcons() reclaims it at the
            // start of the next job.
        }
        bool inplaceRestored = false;
        if(e.mode == QLatin1String("inplace"))
        {
            if(!e.themedPath.isEmpty())
            {
                if(!restoreInplaceStock(e.themedPath))
                {
                    qWarning() << "muoto-launcher: inplace restore failed for" << e.themedPath;
                    // Empty leftovers make Lipstick show a blank tile; drop them so
                    // the icon theme can fall back to another size.
                    if(QFileInfo(e.themedPath).size() <= 0)
                        QFile::remove(e.themedPath);
                    ++failures;
                }
                else
                {
                    touchPath(e.themedPath);
                    inplaceRestored = true;
                }
            }
            // Lipstick caches by Icon= name; inplace never changes Icon=, so touch
            // the desktop to force a reload of the restored PNG.
            if(!touchPath(e.desktop))
                qWarning() << "muoto-launcher: could not touch" << e.desktop;
        }

        saved.set(QString());
        // Only forget the fingerprint when the slot really went back to stock.
        // After a failed restore the file still holds our bytes; clearing the
        // fingerprint made the next apply mistake them for stock and "back them
        // up", burying the real stock artwork for good (found on device: four
        // 86x86 backups held pack art instead of stock).
        if(inplaceRestored)
        {
            MGConfItem fingerprint(LauncherPaths::fingerprintKey(normalizePath(e.themedPath)));
            fingerprint.set(QStringLiteral("<unknown>"));
        }
    }

    clear();
    return failures == 0;
}

void LauncherManifest::clear()
{
    QFile::remove(LauncherPaths::manifestPath());
}

void LauncherManifest::pruneOrphans(const QStringList& existingDesktops)
{
    QList<LauncherManifestEntry> entries;
    if(!load(&entries))
        return;

    QSet<QString> desktops;
    for(const QString& d : existingDesktops)
        desktops.insert(d);

    QList<LauncherManifestEntry> kept;
    bool changed = false;

    for(const LauncherManifestEntry& e : entries)
    {
        if(!desktops.contains(e.desktop))
        {
            if(e.mode == QLatin1String("redirect"))
            {
                if(!e.themedPath.isEmpty() && QFile::exists(e.themedPath))
                    QFile::remove(e.themedPath);
            }
            else if(e.mode == QLatin1String("inplace"))
            {
                if(!e.themedPath.isEmpty())
                    restoreInplaceStock(e.themedPath);
            }
            changed = true;
            continue;
        }
        kept.append(e);
    }

    if(changed)
        save(kept);
}
