#include "iconpaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <pwd.h>
#include <unistd.h>

namespace
{
    const char* kUithemerShare = "/usr/share/sailfishos-uithemer";
    const char* kPackPrefix = "/usr/share/harbour-themepack-";
    const char* kJollaRoot = "/usr/share/themes/sailfish-default/meegotouch";
    const char* kHicolorRoot = "/usr/share/icons/hicolor";
    const char* kApkLauncher = "/home/defaultuser/.local/share/apkd-bridge/launcherIcon";

    const QStringList kNativeSizes = {
        QStringLiteral("256x256"),
        QStringLiteral("172x172"),
        QStringLiteral("128x128"),
        QStringLiteral("108x108"),
        QStringLiteral("86x86"),
    };

    const QStringList kJolla = {
        QStringLiteral("z2.0"),
        QStringLiteral("z1.75"),
        QStringLiteral("z1.5-large"),
        QStringLiteral("z1.5"),
        QStringLiteral("z1.25"),
        QStringLiteral("z1.0"),
    };

    const QStringList kApkSizes = {
        QStringLiteral("192x192"),
        QStringLiteral("128x128"),
        QStringLiteral("86x86"),
    };

    QString packShortName(QString packName)
    {
        static const QString kBare = QStringLiteral("harbour-themepack-");
        if(packName.startsWith(kBare))
            packName = packName.mid(kBare.size());
        return packName;
    }

    QSet<QString> pngBaseNamesInDir(const QString& dirPath)
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

    QString packJollaIconsDir(const QString& packName, const QString& zSize)
    {
        const QString dir = IconPaths::packDir(packName) + QStringLiteral("/jolla/") + zSize
                            + QStringLiteral("/icons/");
        return QDir(dir).exists() ? dir : QString();
    }
}

QString IconPaths::uithemerShare()
{
    return QString::fromLatin1(kUithemerShare);
}

QString IconPaths::backupIconsRoot()
{
    return uithemerShare() + QStringLiteral("/backup/icons");
}

QString IconPaths::tmpDir()
{
    return uithemerShare() + QStringLiteral("/tmp");
}

QString IconPaths::packDir(const QString& packName)
{
    QString name = packName;
    if(!name.startsWith(QLatin1String("harbour-themepack-")))
        name = QString::fromLatin1(kPackPrefix) + packShortName(packName);
    return name;
}

const QStringList& IconPaths::nativeHicolorSizes()
{
    return kNativeSizes;
}

const QStringList& IconPaths::jollaSizes()
{
    return kJolla;
}

const QStringList& IconPaths::apkPackSizes()
{
    return kApkSizes;
}

QString IconPaths::liveJollaIconsDir(const QString& zSize)
{
    return QString::fromLatin1(kJollaRoot) + QLatin1Char('/') + zSize
           + QStringLiteral("/icons/");
}

QString IconPaths::liveNativeAppsDir(const QString& size)
{
    return QString::fromLatin1(kHicolorRoot) + QLatin1Char('/') + size
           + QStringLiteral("/apps/");
}

QString IconPaths::liveApkLauncherDir()
{
    return QString::fromLatin1(kApkLauncher);
}

QString IconPaths::backupJollaIconsDir(const QString& zSize)
{
    return backupIconsRoot() + QStringLiteral("/jolla/") + zSize
           + QStringLiteral("/icons/");
}

QString IconPaths::backupNativeAppsDir(const QString& size)
{
    return backupIconsRoot() + QStringLiteral("/native/") + size
           + QStringLiteral("/apps/");
}

QString IconPaths::backupApkDir()
{
    return backupIconsRoot() + QStringLiteral("/apk/");
}

bool IconPaths::copyFileIgnoreExistingBackup(const QString& src, const QString& dst)
{
    if(!QFileInfo::exists(src))
        return false;
    if(QFileInfo::exists(dst))
        return false;

    QFileInfo di(dst);
    QDir().mkpath(di.absolutePath());

    return QFile::copy(src, dst);
}

bool IconPaths::copyFileExistingOnly(const QString& src, const QString& dst)
{
    if(!QFileInfo::exists(src) || !QFileInfo::exists(dst))
        return false;

    if(QFile::remove(dst))
        return QFile::copy(src, dst);
    return false;
}

int IconPaths::copyPngDirIgnoreExistingBackup(const QString& srcDir, const QString& dstDir)
{
    QDir src(srcDir);
    if(!src.exists())
        return 0;

    QDir().mkpath(dstDir);
    int n = 0;
    const QStringList pngs = src.entryList(QStringList() << QStringLiteral("*.png"),
                                           QDir::Files);
    for(const QString& f : pngs)
    {
        if(copyFileIgnoreExistingBackup(src.absoluteFilePath(f), dstDir + f))
            ++n;
    }
    return n;
}

int IconPaths::copyPngDirExistingOnly(const QString& srcDir, const QString& dstDir)
{
    QDir src(srcDir);
    if(!src.exists())
        return 0;

    QDir().mkpath(dstDir);
    int n = 0;
    const QStringList pngs = src.entryList(QStringList() << QStringLiteral("*.png"),
                                           QDir::Files);
    for(const QString& f : pngs)
    {
        if(copyFileExistingOnly(src.absoluteFilePath(f), dstDir + f))
            ++n;
    }
    return n;
}

void IconPaths::chownApkLauncherTree()
{
    struct passwd* pw = getpwnam("defaultuser");
    if(!pw)
        return;

    const QString root = liveApkLauncherDir();
    QDir d(root);
    if(!d.exists())
        return;

    const QStringList pngs = d.entryList(QStringList() << QStringLiteral("*.png"),
                                          QDir::Files);
    for(const QString& f : pngs)
    {
        const QByteArray path = d.absoluteFilePath(f).toLocal8Bit();
        chown(path.constData(), pw->pw_uid, pw->pw_gid);
    }
}

QString IconPaths::nativeAppsSourceDir(const QString& packName, const QString& size)
{
    const QString apps = packDir(packName) + QStringLiteral("/native/") + size
                         + QStringLiteral("/apps");
    const QFileInfo fi(apps);
    if(!fi.exists())
        return QString();

    if(fi.isSymLink())
    {
        const QString canonical = QFileInfo(fi.canonicalFilePath()).absoluteFilePath();
        return canonical.isEmpty() ? QString() : canonical;
    }

    if(fi.isDir())
        return apps;

    return QString();
}

QSet<QString> IconPaths::packIconKeys(const QString& packName)
{
    QSet<QString> keys;

    for(const QString& size : kNativeSizes)
    {
        const QString dir = nativeAppsSourceDir(packName, size);
        if(!dir.isEmpty())
            keys.unite(pngBaseNamesInDir(dir));
    }

    for(const QString& z : kJolla)
    {
        const QString dir = packJollaIconsDir(packName, z);
        if(!dir.isEmpty())
            keys.unite(pngBaseNamesInDir(dir));
    }

    return keys;
}

QSet<QString> IconPaths::packApkKeys(const QString& packName)
{
    QSet<QString> keys;
    const QString root = packDir(packName);

    for(const QString& size : kApkSizes)
    {
        const QString dir = root + QStringLiteral("/apk/") + size + QLatin1Char('/');
        keys.unite(pngBaseNamesInDir(dir));
    }
    return keys;
}

QSet<QString> IconPaths::liveHicolorAppKeys()
{
    QSet<QString> keys;
    for(const QString& size : kNativeSizes)
        keys.unite(pngBaseNamesInDir(liveNativeAppsDir(size)));
    return keys;
}
