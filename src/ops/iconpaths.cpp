#include "iconpaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <pwd.h>
#include <unistd.h>

namespace
{
    const char *kMuotoShare = "/usr/share/harbour-muoto";
    const char *kPackPrefix = "/usr/share/harbour-themepack-";
    const char *kJollaRoot = "/usr/share/themes/sailfish-default/silica";
    const char *kHicolorRoot = "/usr/share/icons/hicolor";
    const char *kApkLauncher = "/home/defaultuser/.local/share/apkd-bridge/launcherIcon";

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
        const QString kFullPrefix = QString::fromLatin1(kPackPrefix);
        if (packName.startsWith(kFullPrefix))
            packName = packName.mid(kFullPrefix.size());
        else if (packName.startsWith(kBare))
            packName = packName.mid(kBare.size());
        return packName;
    }

    QSet<QString> pngBaseNamesInDir(const QString &dirPath)
    {
        QSet<QString> keys;
        QDir d(dirPath);
        if (!d.exists())
            return keys;

        const QStringList pngs = d.entryList(QStringList() << QStringLiteral("*.png"),
                                             QDir::Files);
        for (const QString &f : pngs)
            keys.insert(QFileInfo(f).completeBaseName());
        return keys;
    }

    QString userHomeDir()
    {
        struct passwd *pw = getpwuid(getuid());
        return pw ? QString::fromUtf8(pw->pw_dir) : QString();
    }

    QString remapNemoHome(const QString &path)
    {
        static const QString kNemoHome = QStringLiteral("/home/nemo");
        const QString home = userHomeDir();
        if (home.isEmpty() || home == kNemoHome)
            return path;
        if (path.startsWith(kNemoHome))
            return home + path.mid(kNemoHome.size());
        return path;
    }

    QString packHomeRoot(const QString &packRoot)
    {
        const QString home = userHomeDir();
        const QString base = QFileInfo(packRoot).fileName();
        if (home.isEmpty() || base.isEmpty())
            return QString();
        return home + QStringLiteral("/.themepack/") + base;
    }

    QString existingCapabilityDir(const QString &path)
    {
        const QFileInfo fi(path);
        if (fi.isDir() && !fi.isSymLink() && fi.exists())
            return fi.absoluteFilePath();
        return QString();
    }

    QString resolveCapabilityDir(const QString &packRoot, const QString &capability)
    {
        const QString entry = packRoot + QLatin1Char('/') + capability;
        const QFileInfo fi(entry);

        QString inspect;
        if (fi.isDir() && !fi.isSymLink())
            inspect = fi.absoluteFilePath();
        else if (fi.isSymLink())
        {
            QString target = fi.symLinkTarget();
            if (!QDir::isAbsolutePath(target))
                target = fi.absoluteDir().absoluteFilePath(target);
            inspect = remapNemoHome(target);
        }
        else
            return QString();

        return existingCapabilityDir(inspect);
    }

    QString packJollaIconsDir(const QString &packName, const QString &zSize)
    {
        const QString jollaRoot = IconPaths::resolvePackCapabilityDir(IconPaths::packDir(packName),
                                                                      QStringLiteral("jolla"));
        if (jollaRoot.isEmpty())
            return QString();

        const QString dir = jollaRoot + QLatin1Char('/') + zSize + QStringLiteral("/icons/");
        return QDir(dir).exists() ? dir : QString();
    }
}

QString IconPaths::muotoShare()
{
    return QString::fromLatin1(kMuotoShare);
}

QString IconPaths::backupIconsRoot()
{
    return muotoShare() + QStringLiteral("/backup/icons");
}

QString IconPaths::tmpDir()
{
    return muotoShare() + QStringLiteral("/tmp");
}

QString IconPaths::packDir(const QString &packName)
{
    return QString::fromLatin1(kPackPrefix) + packShortName(packName);
}

QString IconPaths::resolvePackCapabilityDir(const QString &packRoot, const QString &capability)
{
    QString shareRoot = packRoot;
    if (!shareRoot.startsWith(QString::fromLatin1(kPackPrefix)))
        shareRoot = packDir(packRoot);

    const QString homeRoot = packHomeRoot(shareRoot);
    if (!homeRoot.isEmpty())
    {
        const QString homeCap = resolveCapabilityDir(homeRoot, capability);
        if (!homeCap.isEmpty())
            return homeCap;
    }

    return resolveCapabilityDir(shareRoot, capability);
}

bool IconPaths::packCapabilityUsable(const QString &packRoot, const QString &capability)
{
    const QString resolved = resolvePackCapabilityDir(packRoot, capability);
    if (resolved.isEmpty())
        return false;

    const QDir dir(resolved);
    return !dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot).isEmpty();
}

const QStringList &IconPaths::nativeHicolorSizes()
{
    return kNativeSizes;
}

const QStringList &IconPaths::jollaSizes()
{
    return kJolla;
}

const QStringList &IconPaths::apkPackSizes()
{
    return kApkSizes;
}

QString IconPaths::stockJollaIconsSourceDir(const QString &zSize)
{
    return QString::fromLatin1(kJollaRoot) + QLatin1Char('/') + zSize + QStringLiteral("/icons/");
}

QString IconPaths::liveJollaIconsDir(const QString &zSize)
{
    return stockJollaIconsSourceDir(zSize);
}

QString IconPaths::backupJollaIconsDir(const QString &zSize)
{
    return backupIconsRoot() + QStringLiteral("/jolla/") + zSize + QStringLiteral("/icons/");
}

QString IconPaths::liveNativeAppsDir(const QString &size)
{
    return QString::fromLatin1(kHicolorRoot) + QLatin1Char('/') + size + QStringLiteral("/apps/");
}

QString IconPaths::liveApkLauncherDir()
{
    // helperd runs as root; APK icons always live under defaultuser's home.
    struct passwd *pw = getpwnam("defaultuser");
    if(pw)
    {
        const QString home = QString::fromUtf8(pw->pw_dir);
        if(!home.isEmpty())
            return home + QStringLiteral("/.local/share/apkd-bridge/launcherIcon/");
    }
    return remapNemoHome(QString::fromLatin1(kApkLauncher)) + QLatin1Char('/');
}

QString IconPaths::liveApkApplicationsDir()
{
    struct passwd *pw = getpwnam("defaultuser");
    if(pw)
    {
        const QString home = QString::fromUtf8(pw->pw_dir);
        if(!home.isEmpty())
            return home + QStringLiteral("/.local/share/applications/");
    }
    return QStringLiteral("/home/defaultuser/.local/share/applications/");
}

QString IconPaths::backupNativeAppsDir(const QString &size)
{
    return backupIconsRoot() + QStringLiteral("/native/") + size + QStringLiteral("/apps/");
}

QString IconPaths::backupApkDir()
{
    return backupIconsRoot() + QStringLiteral("/apk/");
}

bool IconPaths::copyFileIgnoreExistingBackup(const QString &src, const QString &dst)
{
    if (!QFileInfo::exists(src))
        return false;
    if (QFileInfo::exists(dst))
        return false;

    QFileInfo di(dst);
    QDir().mkpath(di.absolutePath());

    return QFile::copy(src, dst);
}

bool IconPaths::copyFileExistingOnly(const QString &src, const QString &dst)
{
    if (!QFileInfo::exists(src) || !QFileInfo::exists(dst))
        return false;

    if (QFile::remove(dst))
        return QFile::copy(src, dst);
    return false;
}

int IconPaths::copyPngDirIgnoreExistingBackup(const QString &srcDir, const QString &dstDir)
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
        if (copyFileIgnoreExistingBackup(src.absoluteFilePath(f), dstDir + f))
            ++n;
    }
    return n;
}

int IconPaths::copyPngDirExistingOnly(const QString &srcDir, const QString &dstDir)
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
        if (copyFileExistingOnly(src.absoluteFilePath(f), dstDir + f))
            ++n;
    }
    return n;
}

void IconPaths::chownApkLauncherTree()
{
    struct passwd *pw = getpwnam("defaultuser");
    if (!pw)
        return;

    QDir d(liveApkLauncherDir());
    if (!d.exists())
        return;

    const QStringList pngs = d.entryList(QStringList() << QStringLiteral("*.png"),
                                         QDir::Files);
    for (const QString &f : pngs)
    {
        const QByteArray path = d.absoluteFilePath(f).toLocal8Bit();
        if (chown(path.constData(), pw->pw_uid, pw->pw_gid) != 0)
            continue;
    }
}

QString IconPaths::nativeAppsSourceDir(const QString &packName, const QString &size)
{
    const QString nativeRoot = resolvePackCapabilityDir(packDir(packName), QStringLiteral("native"));
    if (nativeRoot.isEmpty())
        return QString();

    const QString apps = nativeRoot + QLatin1Char('/') + size + QStringLiteral("/apps");
    return QDir(apps).exists() ? apps : QString();
}

QSet<QString> IconPaths::packIconKeys(const QString &packName)
{
    QSet<QString> keys;

    for (const QString &size : kNativeSizes)
    {
        const QString dir = nativeAppsSourceDir(packName, size);
        if (!dir.isEmpty())
            keys.unite(pngBaseNamesInDir(dir));
    }

    for (const QString &z : kJolla)
    {
        const QString dir = packJollaIconsDir(packName, z);
        if (!dir.isEmpty())
            keys.unite(pngBaseNamesInDir(dir));
    }

    return keys;
}

QSet<QString> IconPaths::packApkKeys(const QString &packName)
{
    QSet<QString> keys;
    const QString apkRoot = resolvePackCapabilityDir(packDir(packName), QStringLiteral("apk"));
    if (apkRoot.isEmpty())
        return keys;

    for (const QString &size : kApkSizes)
    {
        const QString dir = apkRoot + QLatin1Char('/') + size + QLatin1Char('/');
        keys.unite(pngBaseNamesInDir(dir));
    }
    return keys;
}

QSet<QString> IconPaths::liveHicolorAppKeys()
{
    QSet<QString> keys;
    for (const QString &size : kNativeSizes)
        keys.unite(pngBaseNamesInDir(liveNativeAppsDir(size)));
    return keys;
}
