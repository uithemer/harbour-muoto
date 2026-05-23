#include "iconpackrunner.h"
#include "iconpaths.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>

bool IconPackRunner::run(const QString& packName) const
{
    const QString packRoot = IconPaths::packDir(packName);
    if(!QDir(packRoot).exists())
    {
        qWarning() << "uithemer: icon pack run: pack not found" << packName << "at" << packRoot;
        return false;
    }

    bool ok = false;

    const QString jollaRoot = IconPaths::resolvePackCapabilityDir(packRoot, QStringLiteral("jolla"));
    const QStringList& jollaCap = IconPaths::jollaSizes();
    for(int i = 0; i < jollaCap.size(); ++i)
    {
        const QString hicolorSize = IconPaths::hicolorSizeForJollaZ(jollaCap.at(i));
        if(hicolorSize.isEmpty())
            continue;

        for(int j = i; j < jollaCap.size(); ++j)
        {
            if(jollaRoot.isEmpty())
                continue;

            const QString src = jollaRoot + QLatin1Char('/') + jollaCap.at(j)
                                + QStringLiteral("/icons/");
            if(!QDir(src).exists())
                continue;

            const int n = IconPaths::copyPngDirExistingOnly(src,
                                                            IconPaths::liveNativeAppsDir(hicolorSize));
            if(n > 0)
                ok = true;
            break;
        }
    }

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

    QDir().mkpath(IconPaths::liveApkLauncherDir());
    const QString apkRoot = IconPaths::resolvePackCapabilityDir(packRoot, QStringLiteral("apk"));
    const QStringList& apkCap = IconPaths::apkPackSizes();
    for(int i = 0; i < apkCap.size(); ++i)
    {
        for(int j = i; j < apkCap.size(); ++j)
        {
            if(apkRoot.isEmpty())
                continue;

            const QString src = apkRoot + QLatin1Char('/') + apkCap.at(j) + QLatin1Char('/');
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

    if(!ok)
        qWarning() << "uithemer: icon pack run produced no copies for" << packName << "root"
                   << packRoot;

    return ok;
}
