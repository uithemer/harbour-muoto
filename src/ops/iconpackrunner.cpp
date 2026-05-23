#include "iconpackrunner.h"
#include "iconpaths.h"

#include <QDir>
#include <QFileInfo>

bool IconPackRunner::run(const QString& packName) const
{
    const QString packRoot = IconPaths::packDir(packName);
    if(!QDir(packRoot).exists())
        return false;

    bool ok = false;

    const QStringList& nativeCap = IconPaths::nativeHicolorSizes();
    for(int i = 0; i < nativeCap.size(); ++i)
    {
        for(int j = i; j < nativeCap.size(); ++j)
        {
            const QString src = IconPaths::nativeAppsSourceDir(packName, nativeCap.at(j));
            if(src.isEmpty())
                continue;

            const QString dst = IconPaths::liveNativeAppsDir(nativeCap.at(i));
            const int n = IconPaths::copyPngDirExistingOnly(src, dst);
            if(n > 0)
                ok = true;
            break;
        }
    }

    if(IconPaths::publishJollaIconsToHicolorCascade(packName) > 0)
        ok = true;

    QDir().mkpath(IconPaths::liveApkLauncherDir());
    const QStringList& apkCap = IconPaths::apkPackSizes();
    for(int i = 0; i < apkCap.size(); ++i)
    {
        for(int j = i; j < apkCap.size(); ++j)
        {
            const QString src = packRoot + QStringLiteral("/apk/") + apkCap.at(j) + QLatin1Char('/');
            if(!QDir(src).exists())
                continue;

            const int n = IconPaths::copyPngDirExistingOnly(src, IconPaths::liveApkLauncherDir());
            if(n > 0)
                ok = true;
            IconPaths::chownApkLauncherTree();
            goto apk_done;
        }
    }
apk_done:

    return ok;
}
