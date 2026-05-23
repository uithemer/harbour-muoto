#include "iconstockbackup.h"
#include "iconpaths.h"

#include <QDir>
#include <QFile>

bool IconStockBackup::backup() const
{
    QDir().mkpath(IconPaths::backupIconsRoot());

    for(const QString& z : IconPaths::jollaSizes())
        IconPaths::copyPngDirIgnoreExistingBackup(IconPaths::liveJollaIconsDir(z),
                                                  IconPaths::backupJollaIconsDir(z));

    for(const QString& size : IconPaths::nativeHicolorSizes())
        IconPaths::copyPngDirIgnoreExistingBackup(IconPaths::liveNativeAppsDir(size),
                                                  IconPaths::backupNativeAppsDir(size));

    {
        QDir apk(IconPaths::liveApkLauncherDir());
        if(apk.exists())
        {
            QDir().mkpath(IconPaths::backupApkDir());
            const QStringList pngs = apk.entryList(QStringList() << QStringLiteral("*.png"),
                                                   QDir::Files);
            for(const QString& f : pngs)
            {
                IconPaths::copyFileIgnoreExistingBackup(apk.absoluteFilePath(f),
                                                        IconPaths::backupApkDir() + f);
            }
        }
    }

    return true;
}

bool IconStockBackup::restore() const
{
    for(const QString& z : IconPaths::jollaSizes())
        IconPaths::copyPngDirExistingOnly(IconPaths::backupJollaIconsDir(z),
                                          IconPaths::liveJollaIconsDir(z));

    for(const QString& size : IconPaths::nativeHicolorSizes())
        IconPaths::copyPngDirExistingOnly(IconPaths::backupNativeAppsDir(size),
                                          IconPaths::liveNativeAppsDir(size));

    {
        QDir().mkpath(IconPaths::liveApkLauncherDir());
        IconPaths::copyPngDirExistingOnly(IconPaths::backupApkDir(),
                                          IconPaths::liveApkLauncherDir());
        IconPaths::chownApkLauncherTree();
    }

    clearBackupTree();
    return true;
}

void IconStockBackup::clearBackupTree() const
{
    QDir root(IconPaths::backupIconsRoot());
    if(root.exists())
        root.removeRecursively();
}
