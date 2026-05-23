#include "iconjollamirror.h"
#include "iconpaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace
{
    int copyLauncherPngsCreateIfMissing(const QString &srcDir, const QString &dstDir)
    {
        QDir src(srcDir);
        if (!src.exists())
            return 0;

        QDir().mkpath(dstDir);
        int n = 0;
        const QStringList pngs = src.entryList(QStringList() << QStringLiteral("*.png"),
                                               QDir::Files);
        for (const QString &f : pngs)
        {
            const QString key = QFileInfo(f).completeBaseName();
            if (!IconPaths::isJollaLauncherIconKey(key))
                continue;

            const QString dst = dstDir + f;
            if (QFileInfo::exists(dst))
                continue;

            if (IconPaths::copyFileIgnoreExistingBackup(src.absoluteFilePath(f), dst))
                ++n;
        }
        return n;
    }
}

void IconJollaMirror::mirrorStockLauncherIcons() const
{
    for (const QString &z : IconPaths::jollaSizes())
    {
        const QString hicolorSize = IconPaths::hicolorSizeForJollaZ(z);
        if (hicolorSize.isEmpty())
            continue;

        const QString src = IconPaths::stockJollaIconsSourceDir(z);
        const QString dst = IconPaths::liveNativeAppsDir(hicolorSize);
        copyLauncherPngsCreateIfMissing(src, dst);
    }
}

void IconJollaMirror::removeStockLauncherIconsFromHicolor() const
{
    for (const QString &size : IconPaths::nativeHicolorSizes())
    {
        QDir d(IconPaths::liveNativeAppsDir(size));
        if (!d.exists())
            continue;

        const QStringList pngs = d.entryList(QStringList() << QStringLiteral("*.png"),
                                               QDir::Files);
        for (const QString &f : pngs)
        {
            if (!IconPaths::isJollaLauncherIconKey(QFileInfo(f).completeBaseName()))
                continue;
            QFile::remove(d.absoluteFilePath(f));
        }
    }
}
