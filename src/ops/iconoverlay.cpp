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

    QStringList livePngsExcludingPack(const QString& dirPath, const QSet<QString>& packKeys)
    {
        QDir d(dirPath);
        if(!d.exists())
            return QStringList();

        QStringList out;
        const QStringList pngs = d.entryList(QStringList() << QStringLiteral("*.png"),
                                             QDir::Files);
        for(const QString& f : pngs)
        {
            if(!packKeys.contains(QFileInfo(f).completeBaseName()))
                out << f;
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

        const QString destPath = IconPaths::liveApkCustomDir() + fileName;
        QDir().mkpath(IconPaths::liveApkCustomDir());
        if(QFile::exists(destPath))
            QFile::remove(destPath);
        if(!out.save(destPath, "PNG"))
        {
            qWarning() << "uithemer: overlay save failed" << destPath << "pack" << packName;
            return false;
        }
        return true;
    }
}

bool IconOverlay::applySfos(const QString& packName) const
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

    const QSet<QString> packNative = IconPaths::packIconKeys(packName);

    int nativeCount = 0;
    for(const QString& liveSize : IconPaths::nativeHicolorSizes())
    {
        const QStringList files =
            livePngsExcludingPack(IconPaths::liveNativeAppsDir(liveSize), packNative);
        for(const QString& file : files)
        {
            if(writeOverlayNative(packName, liveSize, file))
                ++nativeCount;
        }
    }

    if(nativeCount > 0)
        qInfo() << "uithemer: SFOS overlay applied for" << packName << "native" << nativeCount;

    return true;
}

bool IconOverlay::applyApk(const QString& packName, bool* apkIconsTouched) const
{
    if(apkIconsTouched)
        *apkIconsTouched = false;

    const QString launcherDir = IconPaths::liveApkLauncherDir();
    QDir apk(launcherDir);
    if(!apk.exists())
    {
        qWarning() << "uithemer: APK launcherIcon dir missing" << launcherDir;
        return false;
    }

    const QSet<QString> packApk = IconPaths::packApkKeys(packName);
    const QStringList pngs = livePngsExcludingPack(launcherDir, packApk);

    int apkCount = 0;
    for(const QString& f : pngs)
    {
        if(writeOverlayApk(packName, f))
        {
            ++apkCount;
            if(apkIconsTouched)
                *apkIconsTouched = true;
        }
    }

    if(apkCount > 0)
        qInfo() << "uithemer: APK overlay applied for" << packName << "apk" << apkCount;

    return apkCount > 0;
}
