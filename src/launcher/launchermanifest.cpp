#include "launchermanifest.h"
#include "desktopentry.h"
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
    const QString backupPath = LauncherPaths::iconBackupPath(iconPath);
    if(!QFile::exists(backupPath) || QFileInfo(backupPath).size() <= 0)
        return false;

    // Never delete the live icon until the backup is safely copied aside.
    const QString tmpPath = iconPath + QStringLiteral(".muoto-restore");
    QFile::remove(tmpPath);
    if(!QFile::copy(backupPath, tmpPath) || QFileInfo(tmpPath).size() <= 0)
    {
        QFile::remove(tmpPath);
        return false;
    }

    if(!QFile::remove(iconPath) && QFile::exists(iconPath))
    {
        QFile::remove(tmpPath);
        return false;
    }

    if(QFile::rename(tmpPath, iconPath))
        return true;

    const bool copied = QFile::copy(tmpPath, iconPath) && QFileInfo(iconPath).size() > 0;
    QFile::remove(tmpPath);
    return copied;
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
    QDir().mkpath(LauncherPaths::muotoShare());

    QFile file(LauncherPaths::manifestPath());
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    if(file.write(doc.toJson(QJsonDocument::Compact)) < 0)
        return false;

    return true;
}

bool LauncherManifest::appendEntry(const LauncherManifestEntry& entry)
{
    QList<LauncherManifestEntry> entries;
    if(!load(&entries))
        return false;

    for(int i = entries.size() - 1; i >= 0; --i)
    {
        if(entries.at(i).desktop == entry.desktop)
            entries.removeAt(i);
    }

    entries.append(entry);
    return save(entries);
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

        if(e.mode == QLatin1String("redirect"))
        {
            DesktopEntry desktop(e.desktop);
            desktop.setIcon(e.originalIcon);
            if(!desktop.save())
            {
                qWarning() << "muoto-launcher: manifest restore failed for" << e.desktop;
                ++failures;
            }

            if(!e.themedPath.isEmpty() && QFile::exists(e.themedPath))
                QFile::remove(e.themedPath);
        }
        else if(e.mode == QLatin1String("inplace"))
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
                }
            }
            // Lipstick caches by Icon= name; inplace never changes Icon=, so touch
            // the desktop to force a reload of the restored PNG.
            if(!touchPath(e.desktop))
                qWarning() << "muoto-launcher: could not touch" << e.desktop;
        }

        saved.set(QString());
        if(e.mode == QLatin1String("inplace") && !e.themedPath.isEmpty())
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
