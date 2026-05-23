#include "iconoverlay.h"
#include "iconpaths.h"
#include "imageutil.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QSet>
#include <QSize>
#include <QDebug>

namespace
{
    bool parseSize(const QString& size, int& w, int& h)
    {
        const int x = size.indexOf(QLatin1Char('x'));
        if(x <= 0)
            return false;
        bool okW = false;
        bool okH = false;
        w = size.left(x).toInt(&okW);
        h = size.mid(x + 1).toInt(&okH);
        return okW && okH && w > 0 && h > 0;
    }

    QSet<QString> pngBaseNames(const QString& dirPath)
    {
        QSet<QString> keys;
        QDir d(dirPath);
        if(!d.exists())
            return keys;

        const QStringList pngs = d.entryList(QStringList() << QStringLiteral("*.png"),
                                             QDir::Files);
        for(const QString& f : pngs)
            keys.insert(QFileInfo(f).completeBaseName());
        return keys;
    }

    QStringList stockNativeFiles(const QString& liveSize, const QSet<QString>& skipKeys)
    {
        const QString stockDir = IconPaths::liveNativeAppsDir(liveSize);
        const QSet<QString> stock = pngBaseNames(stockDir);
        QStringList out;
        for(const QString& k : stock)
        {
            if(!skipKeys.contains(k))
                out << k + QStringLiteral(".png");
        }
        return out;
    }

    bool writeOverlayNative(const QString& packName, const QString& liveSize,
                            const QString& fileName)
    {
        const QString overlayBase = ImageUtil::randomOverlayBase(IconPaths::packDir(packName));
        if(overlayBase.isEmpty())
            return false;

        const QString stockPath = IconPaths::liveNativeAppsDir(liveSize) + fileName;
        if(!QFileInfo::exists(stockPath))
            return false;

        int w = 0;
        int h = 0;
        if(!parseSize(liveSize, w, h))
            return false;

        const QSize outer(w, h);
        const QSize inner(int(w * 0.6), int(h * 0.6));

        QImage baseImg(overlayBase);
        QImage innerImg(stockPath);
        QImage out = ImageUtil::composite(baseImg, innerImg, outer, inner);
        if(out.isNull())
            return false;

        if(QFile::exists(stockPath))
            QFile::remove(stockPath);
        if(!out.save(stockPath, "PNG"))
        {
            qWarning() << "uithemer: overlay save failed" << stockPath << "pack" << packName;
            return false;
        }
        return true;
    }

    bool writeOverlayApk(const QString& packName, const QString& fileName)
    {
        const QString overlayBase = ImageUtil::randomOverlayBase(IconPaths::packDir(packName));
        if(overlayBase.isEmpty())
            return false;

        const QString stockPath = IconPaths::liveApkLauncherDir() + fileName;
        if(!QFileInfo::exists(stockPath))
            return false;

        const QSize outer(192, 192);
        const QSize inner(122, 122);

        QImage baseImg(overlayBase);
        QImage innerImg(stockPath);
        QImage out = ImageUtil::composite(baseImg, innerImg, outer, inner);
        if(out.isNull())
            return false;

        if(QFile::exists(stockPath))
            QFile::remove(stockPath);
        if(!out.save(stockPath, "PNG"))
        {
            qWarning() << "uithemer: overlay save failed" << stockPath << "pack" << packName;
            return false;
        }
        return true;
    }
}

bool IconOverlay::apply(const QString& packName, bool runPack) const
{
    const QString shareRoot = IconPaths::packDir(packName);
    const QString overlayDir = IconPaths::resolvePackCapabilityDir(shareRoot,
                                                                 QStringLiteral("overlay"));
    if(overlayDir.isEmpty())
    {
        qWarning() << "uithemer: overlay capability not found for" << packName << "share"
                   << shareRoot;
        return false;
    }

    const QStringList overlays = QDir(overlayDir).entryList(
        QStringList() << QStringLiteral("*.png"), QDir::Files);
    if(overlays.isEmpty())
    {
        qWarning() << "uithemer: no overlay PNGs in" << overlayDir;
        return false;
    }

    const QSet<QString> skipNative = runPack ? IconPaths::packIconKeys(packName) : QSet<QString>();
    const QSet<QString> skipApk = runPack ? IconPaths::packApkKeys(packName) : QSet<QString>();

    bool any = false;
    int nativeCount = 0;
    int apkCount = 0;
    int apkLive = 0;
    int apkSkipped = 0;

    for(const QString& liveSize : IconPaths::nativeHicolorSizes())
    {
        const QStringList files = stockNativeFiles(liveSize, skipNative);
        for(const QString& file : files)
        {
            if(writeOverlayNative(packName, liveSize, file))
            {
                any = true;
                ++nativeCount;
            }
        }
    }

    const QString launcherDir = IconPaths::liveApkLauncherDir();
    QDir apk(launcherDir);
    if(!apk.exists())
    {
        qWarning() << "uithemer: APK launcherIcon dir missing" << launcherDir;
    }
    else
    {
        const QStringList pngs = apk.entryList(QStringList() << QStringLiteral("*.png"),
                                               QDir::Files);
        apkLive = pngs.size();
        for(const QString& f : pngs)
        {
            const QString key = QFileInfo(f).completeBaseName();
            if(skipApk.contains(key))
            {
                ++apkSkipped;
                continue;
            }

            if(writeOverlayApk(packName, f))
            {
                any = true;
                ++apkCount;
            }
        }
        IconPaths::chownApkLauncherTree();

        if(apkLive > 0 && apkCount == 0)
        {
            qWarning() << "uithemer: APK overlay wrote 0 of" << apkLive << "launcherIcon PNGs"
                       << "pack" << packName << "runPack" << runPack << "skipped" << apkSkipped;
        }
    }

    if(!any)
        qWarning() << "uithemer: overlay produced no writes for" << packName << "runPack" << runPack;
    else
        qInfo() << "uithemer: overlay applied for" << packName << "runPack" << runPack << "native"
                << nativeCount << "apk" << apkCount << "apkLive" << apkLive << "apkSkipped"
                << apkSkipped;

    return any;
}
