#include "iconoverlay.h"
#include "iconpaths.h"
#include "imageutil.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QSet>
#include <QSize>

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

    QStringList stockOnlyNativeKeys(const QString& packName, const QString& liveSize)
    {
        const QString stockDir = IconPaths::liveNativeAppsDir(liveSize);
        const QSet<QString> stock = pngBaseNames(stockDir);
        const QSet<QString> pack = IconPaths::packIconKeys(packName);
        QStringList out;
        for(const QString& k : stock)
        {
            if(!pack.contains(k))
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
        return out.save(stockPath, "PNG");
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
        return out.save(stockPath, "PNG");
    }
}

bool IconOverlay::apply(const QString& packName) const
{
    const QString overlayDir = IconPaths::packDir(packName) + QStringLiteral("/overlay");
    if(!QDir(overlayDir).exists())
        return false;

    const QStringList overlays = QDir(overlayDir).entryList(
        QStringList() << QStringLiteral("*.png"), QDir::Files);
    if(overlays.isEmpty())
        return false;

    bool any = false;
    const QSet<QString> packApk = IconPaths::packApkKeys(packName);

    for(const QString& liveSize : IconPaths::nativeHicolorSizes())
    {
        const QStringList files = stockOnlyNativeKeys(packName, liveSize);
        for(const QString& file : files)
        {
            if(writeOverlayNative(packName, liveSize, file))
                any = true;
        }
    }

    QDir apk(IconPaths::liveApkLauncherDir());
    if(apk.exists())
    {
        const QStringList pngs = apk.entryList(QStringList() << QStringLiteral("*.png"),
                                               QDir::Files);
        for(const QString& f : pngs)
        {
            if(packApk.contains(QFileInfo(f).completeBaseName()))
                continue;

            if(writeOverlayApk(packName, f))
                any = true;
        }
        IconPaths::chownApkLauncherTree();
    }

    return any;
}
